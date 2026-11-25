CREATE TABLE IF NOT EXISTS client_hosts (
    id CHAR(36) PRIMARY KEY,
    host_number INTEGER NOT NULL UNIQUE,
    host_label VARCHAR(50) NOT NULL UNIQUE,
    machine_name VARCHAR(120) NOT NULL,
    registered_at VARCHAR(26) NOT NULL,
    last_seen_at VARCHAR(26) NOT NULL
);

CREATE TABLE IF NOT EXISTS users (
    id CHAR(36) PRIMARY KEY,
    username VARCHAR(64) NOT NULL UNIQUE,
    full_name VARCHAR(120) NOT NULL,
    email VARCHAR(150) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    system_role VARCHAR(20) NOT NULL DEFAULT 'USER',
    must_change_password TINYINT(1) NOT NULL DEFAULT 1,
    registered_host_id CHAR(36) NULL,
    ui_mode VARCHAR(20) NOT NULL DEFAULT 'RIBBON',
    shortcut_profile VARCHAR(10) NOT NULL DEFAULT 'ES',
    theme VARCHAR(15) NOT NULL DEFAULT 'LIGHT',
    is_active TINYINT(1) NOT NULL DEFAULT 1,
    created_at VARCHAR(26) NOT NULL
);

CREATE TABLE IF NOT EXISTS system_mail_config (
    id INTEGER PRIMARY KEY,
    provider_preset VARCHAR(30) NOT NULL DEFAULT 'CUSTOM',
    smtp_host VARCHAR(150) NOT NULL,
    smtp_port INTEGER NOT NULL DEFAULT 587,
    encryption_mode VARCHAR(20) NOT NULL DEFAULT 'STARTTLS',
    smtp_user VARCHAR(150) NOT NULL,
    smtp_password VARCHAR(255) NOT NULL,
    sender_name VARCHAR(120) NOT NULL DEFAULT 'CollabOffice',
    sender_email VARCHAR(150) NOT NULL,
    updated_at VARCHAR(26) NOT NULL
);

CREATE TABLE IF NOT EXISTS user_groups (
    id CHAR(36) PRIMARY KEY,
    name VARCHAR(80) NOT NULL UNIQUE,
    description VARCHAR(255),
    created_by CHAR(36) NOT NULL,
    created_at VARCHAR(26) NOT NULL
);

CREATE TABLE IF NOT EXISTS user_group_members (
    group_id CHAR(36) NOT NULL,
    user_id CHAR(36) NOT NULL,
    group_role VARCHAR(20) NOT NULL DEFAULT 'MEMBER',
    added_at VARCHAR(26) NOT NULL,
    PRIMARY KEY (group_id, user_id)
);

CREATE TABLE IF NOT EXISTS documents (
    id CHAR(36) PRIMARY KEY,
    title VARCHAR(200) NOT NULL,
    doc_type VARCHAR(20) NOT NULL,
    owner_id CHAR(36) NOT NULL,
    current_version INTEGER NOT NULL DEFAULT 1,
    is_archived TINYINT(1) NOT NULL DEFAULT 0,
    created_at VARCHAR(26) NOT NULL,
    updated_at VARCHAR(26) NOT NULL
);

CREATE TABLE IF NOT EXISTS document_permissions (
    id CHAR(36) PRIMARY KEY,
    document_id CHAR(36) NOT NULL,
    user_id CHAR(36) NULL,
    group_id CHAR(36) NULL,
    access_level VARCHAR(20) NOT NULL,
    granted_by CHAR(36) NOT NULL,
    granted_at VARCHAR(26) NOT NULL
);

CREATE TABLE IF NOT EXISTS element_permissions (
    id CHAR(36) PRIMARY KEY,
    document_id CHAR(36) NOT NULL,
    element_ref VARCHAR(100) NOT NULL,
    user_id CHAR(36) NULL,
    group_id CHAR(36) NULL,
    can_read TINYINT(1) NOT NULL DEFAULT 1,
    can_propose_edit TINYINT(1) NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS doc_blocks (
    id CHAR(36) PRIMARY KEY,
    document_id CHAR(36) NOT NULL,
    parent_block_id CHAR(36) NULL,
    order_index DOUBLE NOT NULL,
    block_type VARCHAR(25) NOT NULL,
    content_text TEXT,
    formatting_json TEXT,
    version_updated INTEGER NOT NULL DEFAULT 1,
    updated_by CHAR(36) NOT NULL,
    updated_at VARCHAR(26) NOT NULL
);

CREATE TABLE IF NOT EXISTS sheet_tabs (
    id CHAR(36) PRIMARY KEY,
    document_id CHAR(36) NOT NULL,
    name VARCHAR(80) NOT NULL,
    tab_order DOUBLE NOT NULL
);

CREATE TABLE IF NOT EXISTS sheet_cells (
    id CHAR(36) PRIMARY KEY,
    sheet_id CHAR(36) NOT NULL,
    row_idx INTEGER NOT NULL,
    col_idx INTEGER NOT NULL,
    raw_value TEXT,
    computed_value TEXT,
    data_type VARCHAR(20) NOT NULL,
    style_json TEXT,
    version_updated INTEGER NOT NULL DEFAULT 1,
    updated_by CHAR(36) NOT NULL,
    updated_at VARCHAR(26) NOT NULL,
    UNIQUE (sheet_id, row_idx, col_idx)
);

CREATE TABLE IF NOT EXISTS change_proposals (
    id CHAR(36) PRIMARY KEY,
    document_id CHAR(36) NOT NULL,
    author_id CHAR(36) NOT NULL,
    base_version INTEGER NOT NULL,
    title VARCHAR(150) NOT NULL,
    summary TEXT,
    status VARCHAR(25) NOT NULL DEFAULT 'DRAFT',
    created_at VARCHAR(26) NOT NULL,
    submitted_at VARCHAR(26) NULL,
    reviewed_by CHAR(36) NULL,
    reviewed_at VARCHAR(26) NULL
);

CREATE TABLE IF NOT EXISTS proposal_items (
    id CHAR(36) PRIMARY KEY,
    proposal_id CHAR(36) NOT NULL,
    target_type VARCHAR(20) NOT NULL,
    target_id CHAR(36) NOT NULL,
    operation VARCHAR(15) NOT NULL,
    old_payload_json TEXT NULL,
    new_payload_json TEXT NULL,
    review_status VARCHAR(20) NOT NULL DEFAULT 'PENDING',
    updated_at VARCHAR(26) NOT NULL
);

CREATE TABLE IF NOT EXISTS proposal_comments (
    id CHAR(36) PRIMARY KEY,
    proposal_id CHAR(36) NOT NULL,
    proposal_item_id CHAR(36) NULL,
    author_id CHAR(36) NOT NULL,
    message TEXT NOT NULL,
    is_resolved TINYINT(1) NOT NULL DEFAULT 0,
    created_at VARCHAR(26) NOT NULL
);

CREATE TABLE IF NOT EXISTS notifications (
    id CHAR(36) PRIMARY KEY,
    recipient_id CHAR(36) NOT NULL,
    sender_id CHAR(36) NOT NULL,
    document_id CHAR(36) NOT NULL,
    proposal_id CHAR(36) NULL,
    notif_type VARCHAR(30) NOT NULL,
    message VARCHAR(255) NOT NULL,
    is_read TINYINT(1) NOT NULL DEFAULT 0,
    created_at VARCHAR(26) NOT NULL
);

CREATE TABLE IF NOT EXISTS audit_history (
    id CHAR(36) PRIMARY KEY,
    document_id CHAR(36) NOT NULL,
    element_type VARCHAR(20) NOT NULL,
    element_id CHAR(36) NOT NULL,
    version_num INTEGER NOT NULL,
    changed_by CHAR(36) NOT NULL,
    proposal_id CHAR(36) NULL,
    old_value TEXT,
    new_value TEXT,
    changed_at VARCHAR(26) NOT NULL
);
