#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <pwd.h>
#include "config.h"

const char *labels[] = {
    "Remote Host (IP or hostname):",
    "Port:",
    "Username:",
    "Local File/Dir Path:",
    "Remote Path:",
    "Cert Location (e.g. ~/.ssh/key):",
    "[ Confirm and Start SCP ]"
};
const int NUM_FIELDS = sizeof(labels) / sizeof(labels[0]);
const int NUM_CHOOSE = NUM_FIELDS - 1;

// 对字符串进行基本的 shell 转义（适用于 Bourne shell）
void shell_escape(const char *src, char *dst, size_t dst_size) {
    if (!src || !dst || dst_size == 0) {
        if (dst) dst[0] = '\0';
        return;
    }

    size_t i = 0, j = 0;
    dst[j++] = '\''; // 用单引号包裹最安全
    while (src[i] != '\0' && j + 2 < dst_size - 1) {
        if (src[i] == '\'') {
            // 单引号不能出现在单引号字符串中，需拆开
            if (j + 4 >= dst_size - 1) break;
            dst[j++] = '\'';
            dst[j++] = '\\';
            dst[j++] = '\'';
            dst[j++] = '\'';
        } else {
            dst[j++] = src[i];
        }
        i++;
    }
    dst[j++] = '\'';
    dst[j] = '\0';
}

// 获取当前字段的指针（用于编辑）
char *get_field_ptr(ScpConfig *cfg, const int idx) {
    switch (idx) {
        case 0: return cfg->host;
        case 1: return cfg->port;
        case 2: return cfg->username;
        case 3: return cfg->local_path;
        case 4: return cfg->remote_path;
        case 5: return cfg->cert_location;
        default: return NULL;
    }
}

// 模拟输入框（简易版）
// void input_string(WINDOW* win, int y, int x, char* buffer, int max_len) {
//     echo();      // 显示输入字符
//     curs_set(1); // 显示光标
//     wmove(win, y, x);
//     wrefresh(win);
//     wgetnstr(win, buffer, max_len - 1);
//     noecho();
//     curs_set(0);
// }

// 改进版 input_string：支持默认值、光标移动、退格等
void input_string(WINDOW *win, const int y, const int x, char *buffer, const int max_len) {
    // 空指针检查
    if (buffer == NULL || max_len <= 0) {
        return;
    }

    // 这里不建议直接转换为int，编译器会有警告
    size_t buf_len = strlen(buffer);
    if (buf_len >= (size_t) max_len - 1) {
        buf_len = (size_t) max_len - 2;
        buffer[buf_len] = '\0';
    }
    int len = (int) buf_len;

    int cursor = len; // 光标初始在末尾

    // 确保不会越界
    if (len >= max_len - 1) {
        len = max_len - 2;
        buffer[len] = '\0';
        cursor = len;
    }

    echo(); // 我们将手动控制显示，其实可以 noecho + 手动刷新
    noecho(); // 更推荐全程 noecho，自己画内容
    curs_set(1); // 显示光标

    while (1) {
        // 清除该行右侧区域（防止旧内容残留）
        mvwhline(win, y, x, ' ', max_len + 10);
        // 显示当前内容
        mvwprintw(win, y, x, "%s", buffer);
        // 移动逻辑光标（ncurses 会自动显示）
        wmove(win, y, x + cursor);
        wrefresh(win);

        const int ch = wgetch(win);

        if (ch == '\n' || ch == KEY_ENTER || ch == 27) {
            // 27: esc键
            break; // 提交
        }
        if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
            if (cursor > 0) {
                // 左侧删除一个字符
                memmove(&buffer[cursor - 1], &buffer[cursor], len - cursor + 1);
                cursor--;
                len--;
            }
        } else if (ch == KEY_DC) {
            // Delete 键
            if (cursor < len) {
                memmove(&buffer[cursor], &buffer[cursor + 1], len - cursor);
                len--;
            }
        } else if (ch == KEY_LEFT) {
            if (cursor > 0) cursor--;
        } else if (ch == KEY_RIGHT) {
            if (cursor < len) cursor++;
        } else if (isprint(ch)) {
            if (len < max_len - 1) {
                // 插入字符
                memmove(&buffer[cursor + 1], &buffer[cursor], len - cursor + 1);
                buffer[cursor] = (char) ch;
                cursor++;
                len++;
            }
        }
        // 自动截断（保险）
        buffer[len] = '\0';
    }

    curs_set(0);
    noecho();

    touchwin(stdscr); // 标记整个屏幕为“脏”，下次 refresh 会重绘
    // refresh();          // 立即重绘（可选，主循环也会做）
}

void save_config_to_user_file(const ScpConfig* cfg) {
    // 获取用户主目录
    struct passwd* pw = getpwuid(getuid());
    if (!pw || !pw->pw_dir) {
        return; // 无法获取 home 目录
    }

    // 构造 ～/.local/share 目录路径
    char share_dir[512];
    snprintf(share_dir, sizeof(share_dir), "%s/.local/share", pw->pw_dir);

    // 创建目录（如果不存在）
    struct stat st = {0};
    if (stat(share_dir, &st) == -1) {
        // 递归创建：先确保 ～/.local 存在
        char local_dir[512];
        snprintf(local_dir, sizeof(local_dir), "%s/.local", pw->pw_dir);
        mkdir(local_dir, 0700);      // 忽略失败（可能已存在）
        mkdir(share_dir, 0700);      // 创建 share 目录
    }

    // 构造配置文件路径
    char config_path[1024];
    snprintf(config_path, sizeof(config_path), "%s/easy_scp.conf", share_dir);

    // 写入配置
    FILE* fp = fopen(config_path, "w");
    if (!fp) return;

    fprintf(fp, "host=%s\n", cfg->host);
    fprintf(fp, "port=%s\n", cfg->port);
    fprintf(fp, "local_path=%s\n", cfg->local_path);
    fprintf(fp, "remote_path=%s\n", cfg->remote_path);

    fclose(fp);
}

int main() {
    ScpConfig config;
    init_config(&config); // 先设默认值
    load_default_config(&config); // 用系统配置覆盖
    // load_config_from_file(&config); // 再用配置文件覆盖
    load_user_config(&config); // 使用用户配置覆盖

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    int selected = 0;

    // 设置 ESC 延迟为 25 毫秒（几乎无感）
    set_escdelay(25);

    while (1) {
        clear();

        mvprintw(2, 2, "=== Visual SCP Configuration ===");
        mvprintw(4, 2, "Use UP/DOWN to select, ENTER to edit, ESC to quit.");

        // 绘制选项
        for (int i = 0; i < NUM_FIELDS; i++) {
            if (i == selected) {
                attron(A_REVERSE);
            }
            mvprintw(NUM_CHOOSE + i, 4, "%s", labels[i]);
            if (i < NUM_CHOOSE) {
                mvprintw(NUM_CHOOSE + i, 40, "%s", get_field_ptr(&config, i));
            }
            if (i == selected) {
                attroff(A_REVERSE);
            }
        }

        refresh();

        const int ch = getch();

        switch (ch) {
            case KEY_UP:
                if (selected > 0) selected--;
                break;
            case KEY_DOWN:
                if (selected < NUM_FIELDS - 1) selected++;
                break;
            case '\n':
                if (selected == NUM_CHOOSE) {
                    // Confirm

                    // 构造 scp 命令（注意：此处仅为演示，未实际执行）
                    printw("\n\nExecuting:\n");

                    // 转义指令
                    char escaped_host[512], escaped_port[64];
                    char escaped_username[64], escaped_cert_location[1024];
                    char escaped_local[1024], escaped_remote[1024];
                    char command[3072];

                    // 转义每个字段
                    shell_escape(config.host, escaped_host, sizeof(escaped_host));
                    shell_escape(config.port, escaped_port, sizeof(escaped_port));
                    shell_escape(config.username, escaped_username, sizeof(escaped_username));
                    shell_escape(config.local_path, escaped_local, sizeof(escaped_local));
                    shell_escape(config.remote_path, escaped_remote, sizeof(escaped_remote));
                    shell_escape(config.cert_location, escaped_cert_location, sizeof(escaped_cert_location));

                    // 拼接最终命令
                    int cmd_len;
                    if (config.cert_location[0] != '\0') {
                        // 带证书
                        cmd_len = snprintf(command, sizeof(command),
                                           "scp -i %s -P %s %s %s@%s:%s",
                                           escaped_cert_location,
                                           escaped_port,
                                           escaped_local,
                                           escaped_username,
                                           escaped_host,
                                           escaped_remote
                        );
                    } else {
                        cmd_len = snprintf(command, sizeof(command),
                                           "scp -P %s %s %s@%s:%s",
                                           escaped_port,
                                           escaped_local,
                                           escaped_username,
                                           escaped_host,
                                           escaped_remote
                        );
                    }

                    // 纠错
                    if (cmd_len < 0 || (size_t) cmd_len >= sizeof(command)) {
                        printw("\n\nError: Command too long!\n");
                        refresh();
                        sleep(2);
                        continue;
                    }

                    // 清屏并显示即将执行的命令
                    clear();
                    printw("Will Executing:\n%s\n\n", command);
                    printw("Press any key to start...\n");
                    refresh();
                    getch(); // 等待用户确认（可选，也可直接执行）

                    // 执行命令
                    endwin(); // 必须先关闭 ncurses，否则 scp 的输出会混乱
                    const int result = system(command);

                    // 重新初始化 ncurses（如果想返回界面）
                    // 但通常执行完 scp 就退出程序更合理
                    if (result == 0) {
                        printf("\nTransfer completed successfully!\n");
                        save_config_to_user_file(&config);
                    } else {
                        printf("\n Transfer failed! (Exit code: %d)\n", WEXITSTATUS(result));
                    }
                    sleep(3);
                    endwin();
                    return 0;
                } else {
                    // 进入编辑模式
                    char *field = get_field_ptr(&config, selected);
                    input_string(stdscr, 6 + selected, 40, field, MAX_INPUT);
                }
                break;
            case 27: // ESC
                endwin();
                return 0;
            default: ;
        }
    }
}
