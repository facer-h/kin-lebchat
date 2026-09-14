-- KinChat 初始数据库结构。
-- 目标环境：MySQL 8.0+

CREATE DATABASE IF NOT EXISTS `User`
  CHARACTER SET utf8mb4
  COLLATE utf8mb4_unicode_ci;

USE `User`;

CREATE TABLE IF NOT EXISTS `User` (
  `UserId` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `UserAccount` VARCHAR(64) NOT NULL,
  `UserPassword` VARCHAR(255) NOT NULL,
  `Nickname` VARCHAR(64) DEFAULT NULL,
  `AvatarUrl` VARCHAR(512) DEFAULT NULL,
  `Status` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `CreatedAt` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `UpdatedAt` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
    ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`UserId`),
  UNIQUE KEY `uk_user_account` (`UserAccount`),
  CONSTRAINT `chk_user_status` CHECK (`Status` IN (0, 1, 2))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `UserFriend` (
  `Id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `UserId` BIGINT UNSIGNED NOT NULL,
  `FriendId` BIGINT UNSIGNED NOT NULL,
  `Status` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `Remark` VARCHAR(64) DEFAULT NULL,
  `CreatedAt` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `UpdatedAt` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
    ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`Id`),
  UNIQUE KEY `uk_user_friend` (`UserId`, `FriendId`),
  KEY `idx_user_friend_friend_id` (`FriendId`),
  KEY `idx_user_friend_status` (`UserId`, `Status`),
  CONSTRAINT `fk_user_friend_user`
    FOREIGN KEY (`UserId`) REFERENCES `User` (`UserId`) ON DELETE CASCADE,
  CONSTRAINT `fk_user_friend_friend`
    FOREIGN KEY (`FriendId`) REFERENCES `User` (`UserId`) ON DELETE CASCADE,
  CONSTRAINT `chk_user_friend_status`
    CHECK (`Status` IN (0, 1, 2, 3, 4)),
  CONSTRAINT `chk_user_friend_not_self` CHECK (`UserId` <> `FriendId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `Conversation` (
  `ConversationId` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `ConversationType` TINYINT UNSIGNED NOT NULL,
  `ConversationName` VARCHAR(64) DEFAULT NULL,
  `OwnerId` BIGINT UNSIGNED DEFAULT NULL,
  `PrivateUserLowId` BIGINT UNSIGNED DEFAULT NULL,
  `PrivateUserHighId` BIGINT UNSIGNED DEFAULT NULL,
  `AvatarUrl` VARCHAR(512) DEFAULT NULL,
  `Status` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `CreatedAt` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `UpdatedAt` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
    ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`ConversationId`),
  UNIQUE KEY `uk_private_conversation`
    (`PrivateUserLowId`, `PrivateUserHighId`),
  KEY `idx_conversation_owner` (`OwnerId`),
  CONSTRAINT `fk_conversation_owner`
    FOREIGN KEY (`OwnerId`) REFERENCES `User` (`UserId`) ON DELETE SET NULL,
  CONSTRAINT `fk_conversation_private_low`
    FOREIGN KEY (`PrivateUserLowId`) REFERENCES `User` (`UserId`),
  CONSTRAINT `fk_conversation_private_high`
    FOREIGN KEY (`PrivateUserHighId`) REFERENCES `User` (`UserId`),
  CONSTRAINT `chk_conversation_type` CHECK (`ConversationType` IN (1, 2)),
  CONSTRAINT `chk_conversation_status` CHECK (`Status` IN (0, 1, 2)),
  CONSTRAINT `chk_private_user_order`
    CHECK (`PrivateUserLowId` IS NULL OR `PrivateUserHighId` IS NULL OR
           `PrivateUserLowId` < `PrivateUserHighId`),
  CONSTRAINT `chk_conversation_membership_shape`
    CHECK ((`ConversationType` = 1 AND `PrivateUserLowId` IS NOT NULL AND
            `PrivateUserHighId` IS NOT NULL) OR
           (`ConversationType` = 2 AND `PrivateUserLowId` IS NULL AND
            `PrivateUserHighId` IS NULL))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `ConversationMember` (
  `ConversationId` BIGINT UNSIGNED NOT NULL,
  `UserId` BIGINT UNSIGNED NOT NULL,
  `Role` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `MuteStatus` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `LastReadMessageId` BIGINT UNSIGNED DEFAULT NULL,
  `JoinedAt` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `LeftAt` DATETIME DEFAULT NULL,
  PRIMARY KEY (`ConversationId`, `UserId`),
  KEY `idx_conversation_member_user` (`UserId`, `LeftAt`),
  KEY `idx_conversation_member_last_read` (`LastReadMessageId`),
  CONSTRAINT `fk_conversation_member_conversation`
    FOREIGN KEY (`ConversationId`) REFERENCES `Conversation` (`ConversationId`)
    ON DELETE CASCADE,
  CONSTRAINT `fk_conversation_member_user`
    FOREIGN KEY (`UserId`) REFERENCES `User` (`UserId`) ON DELETE CASCADE,
  CONSTRAINT `chk_conversation_member_role` CHECK (`Role` IN (0, 1, 2)),
  CONSTRAINT `chk_conversation_member_mute` CHECK (`MuteStatus` IN (0, 1))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `Message` (
  `MessageId` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `ConversationId` BIGINT UNSIGNED NOT NULL,
  `SenderId` BIGINT UNSIGNED NOT NULL,
  `ClientMessageId` CHAR(36) NOT NULL,
  `MessageType` TINYINT UNSIGNED NOT NULL,
  `Content` TEXT NOT NULL,
  `ReplyMessageId` BIGINT UNSIGNED DEFAULT NULL,
  `Status` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `SendTime` DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
  PRIMARY KEY (`MessageId`),
  UNIQUE KEY `uk_sender_client_message` (`SenderId`, `ClientMessageId`),
  KEY `idx_message_conversation` (`ConversationId`, `MessageId`),
  KEY `idx_message_sender` (`SenderId`, `MessageId`),
  KEY `idx_message_reply` (`ReplyMessageId`),
  CONSTRAINT `fk_message_conversation`
    FOREIGN KEY (`ConversationId`) REFERENCES `Conversation` (`ConversationId`),
  CONSTRAINT `fk_message_sender`
    FOREIGN KEY (`SenderId`) REFERENCES `User` (`UserId`),
  CONSTRAINT `fk_message_reply`
    FOREIGN KEY (`ReplyMessageId`) REFERENCES `Message` (`MessageId`)
    ON DELETE SET NULL,
  CONSTRAINT `chk_message_type` CHECK (`MessageType` IN (1, 2, 3, 4, 5, 6)),
  CONSTRAINT `chk_message_status` CHECK (`Status` IN (0, 1, 2))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
