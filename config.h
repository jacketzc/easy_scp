//
// Created by jack on 2026/1/9.
//

#ifndef EASY_SCP_CONFIG_H
#define EASY_SCP_CONFIG_H

#define MAX_INPUT 256


typedef struct {
    char host[MAX_INPUT];
    char port[MAX_INPUT];
    char username[MAX_INPUT];
    char local_path[MAX_INPUT];
    char remote_path[MAX_INPUT];
    char cert_location[MAX_INPUT];
} ScpConfig;




void load_config_from_file(ScpConfig *cfg, const char* config_file_path_str);
void get_user_conf_path(char* buffer, int size);
void load_default_config(ScpConfig *cfg);
void load_user_config(ScpConfig *cfg);
void init_config(ScpConfig *cfg);

#endif //EASY_SCP_CONFIG_H