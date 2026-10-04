-- database schema required to run the project (inferred from the SQL in MysqlDao)
-- usage: mysql -uroot -p123456 < sql/db01.sql

CREATE DATABASE IF NOT EXISTS db01 DEFAULT CHARSET utf8mb4;
USE db01;

CREATE TABLE IF NOT EXISTS user (
  uid   INT          NOT NULL,
  name  VARCHAR(255) NOT NULL,
  email VARCHAR(255) NOT NULL,
  pwd   VARCHAR(255) NOT NULL,
  nick  VARCHAR(255) DEFAULT '',
  `desc` VARCHAR(255) DEFAULT '',
  sex   INT          DEFAULT 0,
  icon  VARCHAR(255) DEFAULT '',
  PRIMARY KEY (uid),
  UNIQUE KEY uk_name (name),
  UNIQUE KEY uk_email (email)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- single-row auto-increment id table
CREATE TABLE IF NOT EXISTS user_id (
  id INT NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

INSERT INTO user_id (id)
SELECT 0 WHERE NOT EXISTS (SELECT 1 FROM user_id);

CREATE TABLE IF NOT EXISTS friend_apply (
  id       INT NOT NULL AUTO_INCREMENT,
  from_uid INT NOT NULL,
  to_uid   INT NOT NULL,
  status   INT NOT NULL DEFAULT 0,
  PRIMARY KEY (id),
  UNIQUE KEY uk_from_to (from_uid, to_uid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS friend (
  self_id   INT NOT NULL,
  friend_id INT NOT NULL,
  back      VARCHAR(255) DEFAULT '',
  PRIMARY KEY (self_id, friend_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

DROP PROCEDURE IF EXISTS reg_user;
DELIMITER //
CREATE PROCEDURE reg_user(IN new_name VARCHAR(255), IN new_email VARCHAR(255), IN new_pwd VARCHAR(255), OUT result INT)
BEGIN
  DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; SET result = -1; END;
  START TRANSACTION;
  IF EXISTS (SELECT 1 FROM `user` WHERE `name` = new_name) THEN
    SET result = 0; COMMIT;
  ELSEIF EXISTS (SELECT 1 FROM `user` WHERE `email` = new_email) THEN
    SET result = 0; COMMIT;
  ELSE
    UPDATE `user_id` SET `id` = `id` + 1;
    SELECT `id` INTO @new_id FROM `user_id`;
    INSERT INTO `user` (`uid`, `name`, `email`, `pwd`) VALUES (@new_id, new_name, new_email, new_pwd);
    SET result = @new_id; COMMIT;
  END IF;
END//
DELIMITER ;
