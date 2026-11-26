/*M!999999\- enable the sandbox mode */ 
-- MariaDB dump 10.19  Distrib 10.5.29-MariaDB, for Linux (x86_64)
--
-- Host: localhost    Database: collabOffice
-- ------------------------------------------------------
-- Server version	10.5.29-MariaDB

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `audit_history`
--

DROP TABLE IF EXISTS `audit_history`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `audit_history` (
  `id` char(36) NOT NULL,
  `document_id` char(36) NOT NULL,
  `element_type` varchar(20) NOT NULL,
  `element_id` char(36) NOT NULL,
  `version_num` int(11) NOT NULL,
  `changed_by` char(36) NOT NULL,
  `proposal_id` char(36) DEFAULT NULL,
  `old_value` text DEFAULT NULL,
  `new_value` text DEFAULT NULL,
  `changed_at` varchar(26) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `audit_history`
--

LOCK TABLES `audit_history` WRITE;
/*!40000 ALTER TABLE `audit_history` DISABLE KEYS */;
INSERT INTO `audit_history` VALUES ('3ffa71ea-df65-429d-a4ea-124cec67eb09','57f7ca0d-2faa-47ea-bd4f-01e4ad633dea','BLOCK','5414a833-2e81-4905-b1c4-c191f4ee1b70',3,'3c5f1ebc-ac0a-4cea-ac57-e2c43d091593',NULL,'','hola mundo','2026-11-26T13:54:14.872Z');
/*!40000 ALTER TABLE `audit_history` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `change_proposals`
--

DROP TABLE IF EXISTS `change_proposals`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `change_proposals` (
  `id` char(36) NOT NULL,
  `document_id` char(36) NOT NULL,
  `author_id` char(36) NOT NULL,
  `base_version` int(11) NOT NULL,
  `title` varchar(150) NOT NULL,
  `summary` text DEFAULT NULL,
  `status` varchar(25) NOT NULL DEFAULT 'DRAFT',
  `created_at` varchar(26) NOT NULL,
  `submitted_at` varchar(26) DEFAULT NULL,
  `reviewed_by` char(36) DEFAULT NULL,
  `reviewed_at` varchar(26) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `change_proposals`
--

LOCK TABLES `change_proposals` WRITE;
/*!40000 ALTER TABLE `change_proposals` DISABLE KEYS */;
/*!40000 ALTER TABLE `change_proposals` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `client_hosts`
--

DROP TABLE IF EXISTS `client_hosts`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `client_hosts` (
  `id` char(36) NOT NULL,
  `host_number` int(11) NOT NULL,
  `host_label` varchar(50) NOT NULL,
  `machine_name` varchar(120) NOT NULL,
  `registered_at` varchar(26) NOT NULL,
  `last_seen_at` varchar(26) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `host_number` (`host_number`),
  UNIQUE KEY `host_label` (`host_label`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `client_hosts`
--

LOCK TABLES `client_hosts` WRITE;
/*!40000 ALTER TABLE `client_hosts` DISABLE KEYS */;
INSERT INTO `client_hosts` VALUES ('e7db7329-0272-448c-8870-81f0c038d848',1,'PC-001','slackware.slackware','2026-11-25T17:18:13.273Z','2026-11-26T13:52:11.075Z');
/*!40000 ALTER TABLE `client_hosts` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `doc_blocks`
--

DROP TABLE IF EXISTS `doc_blocks`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `doc_blocks` (
  `id` char(36) NOT NULL,
  `document_id` char(36) NOT NULL,
  `parent_block_id` char(36) DEFAULT NULL,
  `order_index` double NOT NULL,
  `block_type` varchar(25) NOT NULL,
  `content_text` text DEFAULT NULL,
  `formatting_json` text DEFAULT NULL,
  `version_updated` int(11) NOT NULL DEFAULT 1,
  `updated_by` char(36) NOT NULL,
  `updated_at` varchar(26) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `doc_blocks`
--

LOCK TABLES `doc_blocks` WRITE;
/*!40000 ALTER TABLE `doc_blocks` DISABLE KEYS */;
INSERT INTO `doc_blocks` VALUES ('5414a833-2e81-4905-b1c4-c191f4ee1b70','57f7ca0d-2faa-47ea-bd4f-01e4ad633dea',NULL,1000,'PARAGRAPH','hola mundo','{\"html\":\"<!DOCTYPE HTML PUBLIC \\\"-//W3C//DTD HTML 4.0//EN\\\" \\\"http://www.w3.org/TR/REC-html40/strict.dtd\\\">\\n<html><head><meta name=\\\"qrichtext\\\" content=\\\"1\\\" /><style type=\\\"text/css\\\">\\np, li { white-space: pre-wrap; }\\n</style></head><body>\\n<p style=\\\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><!--StartFragment-->hola mundo<!--EndFragment--></p></body></html>\"}',3,'3c5f1ebc-ac0a-4cea-ac57-e2c43d091593','2026-11-26T13:54:14.872Z');
/*!40000 ALTER TABLE `doc_blocks` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `document_permissions`
--

DROP TABLE IF EXISTS `document_permissions`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `document_permissions` (
  `id` char(36) NOT NULL,
  `document_id` char(36) NOT NULL,
  `user_id` char(36) DEFAULT NULL,
  `group_id` char(36) DEFAULT NULL,
  `access_level` varchar(20) NOT NULL,
  `granted_by` char(36) NOT NULL,
  `granted_at` varchar(26) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `document_permissions`
--

LOCK TABLES `document_permissions` WRITE;
/*!40000 ALTER TABLE `document_permissions` DISABLE KEYS */;
/*!40000 ALTER TABLE `document_permissions` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `documents`
--

DROP TABLE IF EXISTS `documents`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `documents` (
  `id` char(36) NOT NULL,
  `title` varchar(200) NOT NULL,
  `doc_type` varchar(20) NOT NULL,
  `owner_id` char(36) NOT NULL,
  `current_version` int(11) NOT NULL DEFAULT 1,
  `is_archived` tinyint(1) NOT NULL DEFAULT 0,
  `created_at` varchar(26) NOT NULL,
  `updated_at` varchar(26) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `documents`
--

LOCK TABLES `documents` WRITE;
/*!40000 ALTER TABLE `documents` DISABLE KEYS */;
INSERT INTO `documents` VALUES ('57f7ca0d-2faa-47ea-bd4f-01e4ad633dea','prueba','TEXT_DOC','3c5f1ebc-ac0a-4cea-ac57-e2c43d091593',3,0,'2026-11-25T17:59:21.773Z','2026-11-26T13:54:14.872Z');
/*!40000 ALTER TABLE `documents` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `element_permissions`
--

DROP TABLE IF EXISTS `element_permissions`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `element_permissions` (
  `id` char(36) NOT NULL,
  `document_id` char(36) NOT NULL,
  `element_ref` varchar(100) NOT NULL,
  `user_id` char(36) DEFAULT NULL,
  `group_id` char(36) DEFAULT NULL,
  `can_read` tinyint(1) NOT NULL DEFAULT 1,
  `can_propose_edit` tinyint(1) NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `element_permissions`
--

LOCK TABLES `element_permissions` WRITE;
/*!40000 ALTER TABLE `element_permissions` DISABLE KEYS */;
/*!40000 ALTER TABLE `element_permissions` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `notifications`
--

DROP TABLE IF EXISTS `notifications`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `notifications` (
  `id` char(36) NOT NULL,
  `recipient_id` char(36) NOT NULL,
  `sender_id` char(36) NOT NULL,
  `document_id` char(36) NOT NULL,
  `proposal_id` char(36) DEFAULT NULL,
  `notif_type` varchar(30) NOT NULL,
  `message` varchar(255) NOT NULL,
  `is_read` tinyint(1) NOT NULL DEFAULT 0,
  `created_at` varchar(26) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `notifications`
--

LOCK TABLES `notifications` WRITE;
/*!40000 ALTER TABLE `notifications` DISABLE KEYS */;
/*!40000 ALTER TABLE `notifications` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `proposal_comments`
--

DROP TABLE IF EXISTS `proposal_comments`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `proposal_comments` (
  `id` char(36) NOT NULL,
  `proposal_id` char(36) NOT NULL,
  `proposal_item_id` char(36) DEFAULT NULL,
  `author_id` char(36) NOT NULL,
  `message` text NOT NULL,
  `is_resolved` tinyint(1) NOT NULL DEFAULT 0,
  `created_at` varchar(26) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `proposal_comments`
--

LOCK TABLES `proposal_comments` WRITE;
/*!40000 ALTER TABLE `proposal_comments` DISABLE KEYS */;
/*!40000 ALTER TABLE `proposal_comments` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `proposal_items`
--

DROP TABLE IF EXISTS `proposal_items`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `proposal_items` (
  `id` char(36) NOT NULL,
  `proposal_id` char(36) NOT NULL,
  `target_type` varchar(20) NOT NULL,
  `target_id` char(36) NOT NULL,
  `operation` varchar(15) NOT NULL,
  `old_payload_json` text DEFAULT NULL,
  `new_payload_json` text DEFAULT NULL,
  `review_status` varchar(20) NOT NULL DEFAULT 'PENDING',
  `updated_at` varchar(26) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `proposal_items`
--

LOCK TABLES `proposal_items` WRITE;
/*!40000 ALTER TABLE `proposal_items` DISABLE KEYS */;
/*!40000 ALTER TABLE `proposal_items` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `sheet_cells`
--

DROP TABLE IF EXISTS `sheet_cells`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `sheet_cells` (
  `id` char(36) NOT NULL,
  `sheet_id` char(36) NOT NULL,
  `row_idx` int(11) NOT NULL,
  `col_idx` int(11) NOT NULL,
  `raw_value` text DEFAULT NULL,
  `computed_value` text DEFAULT NULL,
  `data_type` varchar(20) NOT NULL,
  `style_json` text DEFAULT NULL,
  `version_updated` int(11) NOT NULL DEFAULT 1,
  `updated_by` char(36) NOT NULL,
  `updated_at` varchar(26) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `sheet_id` (`sheet_id`,`row_idx`,`col_idx`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `sheet_cells`
--

LOCK TABLES `sheet_cells` WRITE;
/*!40000 ALTER TABLE `sheet_cells` DISABLE KEYS */;
/*!40000 ALTER TABLE `sheet_cells` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `sheet_tabs`
--

DROP TABLE IF EXISTS `sheet_tabs`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `sheet_tabs` (
  `id` char(36) NOT NULL,
  `document_id` char(36) NOT NULL,
  `name` varchar(80) NOT NULL,
  `tab_order` double NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `sheet_tabs`
--

LOCK TABLES `sheet_tabs` WRITE;
/*!40000 ALTER TABLE `sheet_tabs` DISABLE KEYS */;
/*!40000 ALTER TABLE `sheet_tabs` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `system_mail_config`
--

DROP TABLE IF EXISTS `system_mail_config`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `system_mail_config` (
  `id` int(11) NOT NULL,
  `provider_preset` varchar(30) NOT NULL DEFAULT 'CUSTOM',
  `smtp_host` varchar(150) NOT NULL,
  `smtp_port` int(11) NOT NULL DEFAULT 587,
  `encryption_mode` varchar(20) NOT NULL DEFAULT 'STARTTLS',
  `smtp_user` varchar(150) NOT NULL,
  `smtp_password` varchar(255) NOT NULL,
  `sender_name` varchar(120) NOT NULL DEFAULT 'CollabOffice',
  `sender_email` varchar(150) NOT NULL,
  `updated_at` varchar(26) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `system_mail_config`
--

LOCK TABLES `system_mail_config` WRITE;
/*!40000 ALTER TABLE `system_mail_config` DISABLE KEYS */;
/*!40000 ALTER TABLE `system_mail_config` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `user_group_members`
--

DROP TABLE IF EXISTS `user_group_members`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `user_group_members` (
  `group_id` char(36) NOT NULL,
  `user_id` char(36) NOT NULL,
  `group_role` varchar(20) NOT NULL DEFAULT 'MEMBER',
  `added_at` varchar(26) NOT NULL,
  PRIMARY KEY (`group_id`,`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `user_group_members`
--

LOCK TABLES `user_group_members` WRITE;
/*!40000 ALTER TABLE `user_group_members` DISABLE KEYS */;
/*!40000 ALTER TABLE `user_group_members` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `user_groups`
--

DROP TABLE IF EXISTS `user_groups`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `user_groups` (
  `id` char(36) NOT NULL,
  `name` varchar(80) NOT NULL,
  `description` varchar(255) DEFAULT NULL,
  `created_by` char(36) NOT NULL,
  `created_at` varchar(26) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `user_groups`
--

LOCK TABLES `user_groups` WRITE;
/*!40000 ALTER TABLE `user_groups` DISABLE KEYS */;
/*!40000 ALTER TABLE `user_groups` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `users`
--

DROP TABLE IF EXISTS `users`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `users` (
  `id` char(36) NOT NULL,
  `username` varchar(64) NOT NULL,
  `full_name` varchar(120) NOT NULL,
  `email` varchar(150) NOT NULL,
  `password_hash` varchar(255) NOT NULL,
  `system_role` varchar(20) NOT NULL DEFAULT 'USER',
  `must_change_password` tinyint(1) NOT NULL DEFAULT 1,
  `registered_host_id` char(36) DEFAULT NULL,
  `ui_mode` varchar(20) NOT NULL DEFAULT 'RIBBON',
  `shortcut_profile` varchar(10) NOT NULL DEFAULT 'ES',
  `theme` varchar(15) NOT NULL DEFAULT 'LIGHT',
  `is_active` tinyint(1) NOT NULL DEFAULT 1,
  `created_at` varchar(26) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `username` (`username`),
  UNIQUE KEY `email` (`email`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `users`
--

LOCK TABLES `users` WRITE;
/*!40000 ALTER TABLE `users` DISABLE KEYS */;
INSERT INTO `users` VALUES ('3c5f1ebc-ac0a-4cea-ac57-e2c43d091593','admin','Administrador','develslack@gmail.com','sha256$6c7ed8a8-d174-47$fde3571ce4575a23360ce98d1833c875141bba36fb9c7d49e67dedc33ce04d0d','ADMIN',0,'e7db7329-0272-448c-8870-81f0c038d848','RIBBON','ES','LIGHT',1,'2026-11-25T17:19:02.852Z');
/*!40000 ALTER TABLE `users` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping routines for database 'collabOffice'
--
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2026-11-26 11:26:51
