#include "MysqlDao.h"
#include "ConfigMgr.h"
#include <algorithm>
#include <chrono>
#include <limits>

namespace
{
constexpr int kTransactionRetryLimit = 3;

// 1213 is the deadlock victim; 1205 is a lock-wait timeout. MySQL has already rolled
// the transaction back in both cases, so the whole unit of work can be retried.
bool IsRetryableTransactionError(int code)
{
    return code == 1213 || code == 1205;
}
} // namespace

MysqlDao::MysqlDao()
{
    auto& cfg = ConfigMgr::Inst();
    const auto& host = cfg["Mysql"]["Host"];
    const auto& port = cfg["Mysql"]["Port"];
    const auto& pwd = cfg["Mysql"]["Passwd"];
    const auto& schema = cfg["Mysql"]["Schema"];
    const auto& user = cfg["Mysql"]["User"];
    pool_.reset(new MySqlPool(host + ":" + port, user, pwd, schema, 5));
}

MysqlDao::~MysqlDao()
{
    pool_->Close();
}

int MysqlDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
    auto con = pool_->getConnection();
    try
    {
        if (con == nullptr)
        {
            return false;
        }
        // prepare to call the stored procedure
        std::unique_ptr<sql::PreparedStatement> stmt(con->_con->prepareStatement("CALL reg_user(?,?,?,@result)"));
        // set the input parameters
        stmt->setString(1, name);
        stmt->setString(2, email);
        stmt->setString(3, pwd);

        // since PreparedStatement does not register output params directly, use a session variable or another way to fetch the output value

        // call the stored procedure
        stmt->execute();
        // if the stored procedure set a session variable or the output can be fetched otherwise, run a SELECT here to read it
        // For example, if the stored procedure sets a session variable @result for the output, fetch it like this:
        std::unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
        if (res->next())
        {
            int result = res->getInt("result");
            std::cout << "Result: " << result << std::endl;
            pool_->returnConnection(std::move(con));
            return result;
        }
        pool_->returnConnection(std::move(con));
        return -1;
    }
    catch (sql::SQLException& e)
    {
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return -1;
    }
}

bool MysqlDao::CheckEmail(const std::string& name, const std::string& email)
{
    auto con = pool_->getConnection();
    try
    {
        if (con == nullptr)
        {
            pool_->returnConnection(std::move(con));
            return false;
        }

        // prepare the query statement
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT email FROM user WHERE name = ?"));

        // bind parameters
        pstmt->setString(1, name);

        // execute the query
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        // iterate the result set
        while (res->next())
        {
            std::cout << "Check Email: " << res->getString("email") << std::endl;

            if (email != res->getString("email"))
            {
                pool_->returnConnection(std::move(con));
                return false;
            }
            pool_->returnConnection(std::move(con));
            return true;
        }

        pool_->returnConnection(std::move(con));
        return false;
    }
    catch (sql::SQLException& e)
    {
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MysqlDao::UpdatePwd(const std::string& name, const std::string& newpwd)
{
    auto con = pool_->getConnection();
    try
    {
        if (con == nullptr)
        {
            pool_->returnConnection(std::move(con));
            return false;
        }

        // prepare the query statement
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));

        // bind parameters
        pstmt->setString(2, name);
        pstmt->setString(1, newpwd);

        // execute the update
        int updateCount = pstmt->executeUpdate();

        std::cout << "Updated rows: " << updateCount << std::endl;
        pool_->returnConnection(std::move(con));
        return true;
    }
    catch (sql::SQLException& e)
    {
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MysqlDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo)
{
    auto con = pool_->getConnection();
    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
    });

    try
    {
        if (con == nullptr)
        {
            std::cerr << "Failed to get connection from pool." << std::endl;
            return false;
        }

        // prepare the SQL statement
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT pwd, email, uid FROM user WHERE email = ?"));
        pstmt->setString(1, email); // replace email with the one you want to query

        // execute the query
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::string origin_pwd = "";
        // iterate the result set
        if (res->next())
        {
            origin_pwd = res->getString("pwd");
            if (pwd != origin_pwd)
            {
                std::cerr << "Password does not match." << std::endl;
                return false;
            }

            userInfo.email = res->getString("email");
            userInfo.uid = res->getInt("uid");
            userInfo.pwd = origin_pwd;
            return true;
        }
        else
        {
            std::cerr << "No user found with the given name." << std::endl;
            return false;
        }
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MysqlDao::AddFriendApply(const int& from, const int& to)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
        return false;

    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
    });

    try
    {
        // prepare the SQL statement
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("INSERT INTO friend_apply (from_uid, to_uid) values (?,?) "
                                                                                  "ON DUPLICATE KEY UPDATE from_uid = from_uid, to_uid = to_uid"));
        pstmt->setInt(1, from);
        pstmt->setInt(2, to);

        // execute the update
        int rowAffected = pstmt->executeUpdate();
        if (rowAffected < 0)
            return false;
        return true;
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException :" << e.what();
        std::cerr << "(MySQL err code : " << e.getErrorCode();
        std::cerr << "SQLState :" << e.getSQLState() << ")" << std::endl;
        return false;
    }
    return true;
}

bool MysqlDao::AuthFriendApply(const int& from, const int& to)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
        return false;

    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
    });
    try
    {
        // approving requires an apply from the other side; an already-approved one still
        // counts, so a retry after a failed AddFriend can finish creating the friendship
        std::unique_ptr<sql::PreparedStatement> query(con->_con->prepareStatement("SELECT 1 FROM friend_apply "
                                                                                  "WHERE from_uid = ? AND to_uid = ?"));
        query->setInt(1, to);
        query->setInt(2, from);
        std::unique_ptr<sql::ResultSet> res(query->executeQuery());
        if (!res->next())
        {
            return false;
        }

        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE friend_apply SET status = 1 "
                                                                                  "WHERE from_uid = ? AND to_uid = ?"));
        pstmt->setInt(1, to);
        pstmt->setInt(2, from);
        pstmt->executeUpdate();
        return true;
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
    return true;
}

bool MysqlDao::AddFriend(const int& from, const int& to, std::string back_name)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer([this, &con]() {
        // restore autocommit so the pooled connection is not left in
        // manual-commit mode (which would poison the next borrower)
        if (con && con->_con)
        {
            try
            {
                con->_con->setAutoCommit(true);
            }
            catch (...)
            {
            }
        }
        pool_->returnConnection(std::move(con));
    });

    for (int attempt = 0; attempt < kTransactionRetryLimit; ++attempt)
    {
        try
        {
            // begin the transaction
            con->_con->setAutoCommit(false);

            // Reciprocal approvals insert the same two rows. Writing both in one statement,
            // ordered by self_id, gives every transaction the same lock order, so the two
            // sides serialize instead of deadlocking on (A,B) vs (B,A).
            const bool from_first = from < to;
            const int first_uid = from_first ? from : to;
            const int second_uid = from_first ? to : from;
            const std::string first_back = from_first ? back_name : "";
            const std::string second_back = from_first ? "" : back_name;

            std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id, back) "
                                                                                      "VALUES (?, ?, ?), (?, ?, ?)"));
            pstmt->setInt(1, first_uid);
            pstmt->setInt(2, second_uid);
            pstmt->setString(3, first_back);
            pstmt->setInt(4, second_uid);
            pstmt->setInt(5, first_uid);
            pstmt->setString(6, second_back);

            // execute the update
            int rowAffected = pstmt->executeUpdate();
            if (rowAffected < 0)
            {
                con->_con->rollback();
                return false;
            }

            // commit the transaction
            con->_con->commit();
            std::cout << "addfriend insert friends success" << std::endl;

            return true;
        }
        catch (sql::SQLException& e)
        {
            // on error, roll back the transaction
            try
            {
                con->_con->rollback();
            }
            catch (...)
            {
            }

            // INSERT IGNORE makes the retry idempotent: both rows are keyed by
            // (self_id, friend_id), so a re-run cannot create duplicates.
            if (IsRetryableTransactionError(e.getErrorCode()) && attempt + 1 < kTransactionRetryLimit)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10 * (1 << attempt)));
                continue;
            }

            std::cerr << "SQLException: " << e.what();
            std::cerr << " (MySQL error code: " << e.getErrorCode();
            std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
            return false;
        }
    }

    return false;
}

std::shared_ptr<UserInfo> MysqlDao::GetUser(int uid)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return nullptr;
    }

    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
    });

    try
    {
        // prepare the SQL statement
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE uid = ?"));
        pstmt->setInt(1, uid); // replace uid with the one you want to query

        // execute the query
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> user_ptr = nullptr;
        // iterate the result set
        while (res->next())
        {
            user_ptr.reset(new UserInfo);
            user_ptr->pwd = res->getString("pwd");
            user_ptr->email = res->getString("email");
            user_ptr->name = res->getString("name");
            user_ptr->nick = res->getString("nick");
            user_ptr->desc = res->getString("desc");
            user_ptr->sex = res->getInt("sex");
            user_ptr->icon = res->getString("icon");
            user_ptr->uid = uid;
            break;
        }
        return user_ptr;
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return nullptr;
    }
}

std::shared_ptr<UserInfo> MysqlDao::GetUser(std::string name)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return nullptr;
    }

    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
    });

    try
    {
        // prepare the SQL statement
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE name = ?"));
        pstmt->setString(1, name); // replace uid with the one you want to query

        // execute the query
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> user_ptr = nullptr;
        // iterate the result set
        while (res->next())
        {
            user_ptr.reset(new UserInfo);
            user_ptr->pwd = res->getString("pwd");
            user_ptr->email = res->getString("email");
            user_ptr->name = res->getString("name");
            user_ptr->nick = res->getString("nick");
            user_ptr->desc = res->getString("desc");
            user_ptr->sex = res->getInt("sex");
            user_ptr->uid = res->getInt("uid");
            break;
        }
        return user_ptr;
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return nullptr;
    }
}

bool MysqlDao::GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
    });

    try
    {
        // prepare the SQL statement: return the list by start id and limit
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("select apply.from_uid, apply.status, user.name, "
                                                                                  "user.nick, user.sex from friend_apply as apply join user on apply.from_uid = user.uid where apply.to_uid = ? "
                                                                                  "and apply.id > ? order by apply.id ASC LIMIT ? "));

        pstmt->setInt(1, touid); // replace uid with the one you want to query
        pstmt->setInt(2, begin); // start id
        pstmt->setInt(3, limit); // offset
        // execute the query
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        // iterate the result set
        while (res->next())
        {
            auto name = res->getString("name");
            auto uid = res->getInt("from_uid");
            auto status = res->getInt("status");
            auto nick = res->getString("nick");
            auto sex = res->getInt("sex");
            auto apply_ptr = std::make_shared<ApplyInfo>(uid, name, "", "", nick, sex, status);
            applyList.push_back(apply_ptr);
        }
        return true;
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MysqlDao::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_info_list)
{

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
    });

    try
    {
        // prepare the SQL statement: return the list by start id and limit
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("select * from friend where self_id = ? "));

        pstmt->setInt(1, self_id); // replace uid with the one you want to query

        // execute the query
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        // iterate the result set
        while (res->next())
        {
            auto friend_id = res->getInt("friend_id");
            auto back = res->getString("back");
            // query the info for friend_id once more
            auto user_info = GetUser(friend_id);
            if (user_info == nullptr)
            {
                continue;
            }

            user_info->back = user_info->name;
            user_info_list.push_back(user_info);
        }
        return true;
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }

    return true;
}

bool MysqlDao::AddChatMessages(int from_uid, int to_uid,
                               const std::vector<std::pair<std::string, std::string>>& messages)
{
    if (messages.empty())
        return true;

    auto con = pool_->getConnection();
    if (con == nullptr)
        return false;

    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
    });

    try
    {
        // INSERT IGNORE makes a retried batch idempotent: the msg_id is generated once by
        // the sender, so a repeat delivery must not create a second history row
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("INSERT IGNORE INTO chat_message (msg_id, from_uid, to_uid, content) "
                                        "VALUES (?, ?, ?, ?)"));
        for (const auto& msg : messages)
        {
            pstmt->setString(1, msg.first);
            pstmt->setInt(2, from_uid);
            pstmt->setInt(3, to_uid);
            pstmt->setString(4, msg.second);
            if (pstmt->executeUpdate() < 0)
                return false;
        }
        return true;
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MysqlDao::GetChatHistory(int self_uid, int peer_uid, long long before_id, int limit,
                              std::vector<std::shared_ptr<ChatMessageInfo>>& messages, bool& has_more)
{
    messages.clear();
    has_more = false;

    auto con = pool_->getConnection();
    if (con == nullptr)
        return false;

    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
    });

    try
    {
        // newest first so the first page is the recent conversation; one extra row tells the
        // caller whether an older page exists
        const long long cursor =
            before_id > 0 ? before_id : std::numeric_limits<long long>::max();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("SELECT id, msg_id, from_uid, to_uid, content FROM chat_message "
                                        "WHERE ((from_uid = ? AND to_uid = ?) OR (from_uid = ? AND to_uid = ?)) "
                                        "AND id < ? ORDER BY id DESC LIMIT ?"));
        pstmt->setInt(1, self_uid);
        pstmt->setInt(2, peer_uid);
        pstmt->setInt(3, peer_uid);
        pstmt->setInt(4, self_uid);
        pstmt->setInt64(5, cursor);
        pstmt->setInt(6, limit + 1);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            messages.push_back(std::make_shared<ChatMessageInfo>(
                res->getInt64("id"), res->getString("msg_id"), res->getInt("from_uid"),
                res->getInt("to_uid"), res->getString("content")));
        }

        if (static_cast<int>(messages.size()) > limit)
        {
            has_more = true;
            messages.pop_back();
        }
        std::reverse(messages.begin(), messages.end());
        return true;
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}
