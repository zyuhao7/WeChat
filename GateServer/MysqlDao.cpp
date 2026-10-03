#include "MysqlDao.h"
#include "ConfigMgr.h"

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

MysqlDao::~MysqlDao() {
	pool_->Close();
}

int MysqlDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			return false;
		}
		// prepare to call the stored procedure
		std::unique_ptr <sql::PreparedStatement> stmt(con->_con->prepareStatement("CALL reg_user(?,?,?,@result)"));
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
		if (res->next()) {
			int result = res->getInt("result");
			std::cout << "Result: " << result << std::endl;
			pool_->returnConnection(std::move(con));
			return result;
		}
		pool_->returnConnection(std::move(con));
		return -1;
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return -1;
	}
}

int MysqlDao::RegUserTransaction(const std::string& name, const std::string& email, const std::string& pwd, const std::string& icon)
{
	auto con = pool_->getConnection();
	if (con == nullptr)
		return 0;

	Defer defer([this, &con]() {
		// restore autocommit so the pooled connection is not left in
		// manual-commit mode (which would poison the next borrower)
		if (con && con->_con) {
			try { con->_con->setAutoCommit(true); } catch (...) {}
		}
		pool_->returnConnection(std::move(con));
		});
	try
	{
		// begin the transaction
		con->_con->setAutoCommit(false);

		// run the first DB query: find the user by email
		std::unique_ptr<sql::PreparedStatement> pstmt_email(con->_con->prepareStatement("select 1 from user where email = ?"));
	
		// bind parameters
		pstmt_email->setString(1, email);

		// execute the query
		std::unique_ptr<sql::ResultSet> res_email(pstmt_email->executeQuery());

		auto email_exist = res_email->next();
		if (email_exist)
		{
			con->_con->rollback();
			std::cout << "email " << email << "exist";
			return 0;
		}

		// prepare to check whether the username is duplicated
		std::unique_ptr<sql::PreparedStatement> pstmt_name(con->_con->prepareStatement("select 1 from user where name = ?"));
		pstmt_name->setString(1, name);
		std::unique_ptr<sql::ResultSet> res_name(pstmt_name->executeQuery());

		auto name_exist = res_name->next();
		if (name_exist)
		{
			con->_con->rollback();
			std::cout << "name " << name << " exist";
		}

		// prepare to update the user id
		std::unique_ptr<sql::PreparedStatement> pstmt_upid(con->_con->prepareStatement("update user_id set id = id + 1"));
		pstmt_upid->executeUpdate();

		std::unique_ptr<sql::PreparedStatement> pstmt_uid(con->_con->prepareStatement("select id from user_id"));
		std::unique_ptr<sql::ResultSet> res_uid(pstmt_uid->executeQuery());
		int newId = 0;

		if (res_uid->next())
		{
			newId = res_uid->getInt("id");
		}
		else
		{
			std::cout << "select id from user_id failed " << std::endl;
			con->_con->rollback();
			return -1;
		}
		// insert user info
		std::unique_ptr<sql::PreparedStatement> pstmt_insert(con->_con->prepareStatement("insert into user(uid, name,email, pwd, nick, icon)"
			"values(?,?,?,?,?,?)"));
		pstmt_insert->setInt(1, newId);
		pstmt_insert->setString(2, name);
		pstmt_insert->setString(3, email);
		pstmt_insert->setString(4, pwd);
		pstmt_insert->setString(5, name);
		pstmt_insert->setString(6, icon);
		//insert
		pstmt_insert->executeUpdate();
		con->_con->commit();
		std::cout << "newUser insert into user success!" << std::endl;
		return newId;
	}
	catch (const sql::SQLException& e)
	{
		if (con)
		{
			con->_con->rollback();
		}
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return -1;
	}
}

bool MysqlDao::CheckEmail(const std::string& name, const std::string& email) {
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
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
		while (res->next()) {
			std::cout << "Check Email: " << res->getString("email") << std::endl;

			if (email != res->getString("email")) {
				pool_->returnConnection(std::move(con));
				return false;
			}
			pool_->returnConnection(std::move(con));
			return true;
		}

		pool_->returnConnection(std::move(con));
		return false;
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::UpdatePwd(const std::string& name, const std::string& newpwd) {
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
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
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
	auto con = pool_->getConnection();
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		if (con == nullptr) {
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
		if (res->next()) {
			origin_pwd = res->getString("pwd");

			if (pwd != origin_pwd) {
				std::cerr << "Password does not match." << std::endl;
				return false;
			}

			userInfo.email = res->getString("email");
			userInfo.uid = res->getInt("uid");
			userInfo.pwd = origin_pwd;
			return true;
		}
		else {
			std::cerr << "No user found with the given name." << std::endl;
			return false;
		}
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}