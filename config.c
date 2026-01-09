//
// Created by jack on 2026/1/9.
//

#include "config.h"
#include <string.h>
#include <pwd.h>
#include <unistd.h>
#include <stdio.h>
#define CONFIG_FILE "/etc/easy_scp.conf"

void get_user_conf_path(char* buffer, const int size) {
    struct passwd* pw = getpwuid(getuid());
    if (!pw || !pw->pw_dir) {
        strncpy(buffer, "", 0);
        return;
    }

    snprintf(buffer, size, "%s/.local/share/easy_scp.conf", pw->pw_dir);
}

void load_config_from_file(ScpConfig *cfg, const char* config_file_path_str) {
    FILE *fp = fopen(config_file_path_str, "r");
    if (!fp) {
        // 文件不存在或无法打开，静默忽略（使用默认值）
        return;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        // 去掉换行符
        line[strcspn(line, "\r\n")] = '\0';

        // 跳过空行和注释（以 # 开头）
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        // 查找 '=' 分隔符
        char *eq = strchr(line, '=');
        if (!eq) continue;

        *eq = '\0'; // 分割 key 和 value
        char *key = line;
        char *value = eq + 1;

        // 去除 key/value 前后空格（简单处理）
        while (*key == ' ' || *key == '\t') key++;
        while (*value == ' ' || *value == '\t') value++;

        // 确保 key 和 value 非空
        if (key[0] == '\0' || value[0] == '\0') continue;

        // 根据 key 覆盖对应字段（只覆盖存在的 key）
        if (strcmp(key, "host") == 0) {
            strncpy(cfg->host, value, MAX_INPUT - 1);
            cfg->host[MAX_INPUT - 1] = '\0';
        } else if (strcmp(key, "port") == 0) {
            strncpy(cfg->port, value, MAX_INPUT - 1);
            cfg->port[MAX_INPUT - 1] = '\0';
        } else if (strcmp(key, "local_path") == 0) {
            strncpy(cfg->local_path, value, MAX_INPUT - 1);
            cfg->local_path[MAX_INPUT - 1] = '\0';
        } else if (strcmp(key, "remote_path") == 0) {
            strncpy(cfg->remote_path, value, MAX_INPUT - 1);
            cfg->remote_path[MAX_INPUT - 1] = '\0';
        } else if (strcmp(key, "username") == 0) {
            strncpy(cfg->username, value, MAX_INPUT - 1);
            cfg->username[MAX_INPUT - 1] = '\0';
        } else if (strcmp(key, "cert_location") == 0) {
            strncpy(cfg->cert_location, value, MAX_INPUT - 1);
            cfg->cert_location[MAX_INPUT - 1] = '\0';
        }
        // 忽略未知 key
    }

    fclose(fp);
}

void load_default_config(ScpConfig *cfg) {
    load_config_from_file(cfg, CONFIG_FILE);
}

void load_user_config(ScpConfig *cfg) {
    char user_config_filepath[512];
    get_user_conf_path(user_config_filepath, 512);
    load_config_from_file(cfg, user_config_filepath);
}

void init_config(ScpConfig *cfg) {
    strcpy(cfg->host, "192.168.1.1");
    strcpy(cfg->port, "22");
    strcpy(cfg->username, "kdev");
    strcpy(cfg->local_path, "./file.txt");
    strcpy(cfg->remote_path, "/tmp");
    strcpy(cfg->cert_location, "");
}