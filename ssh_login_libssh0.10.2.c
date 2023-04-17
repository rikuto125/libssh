#include <libssh/libssh.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include <libssh/libssh.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h> // select関数を使うために必要なヘッダーファイルを追加
#include <unistd.h> // read関数を使うために必要なヘッダーファイルを追加

int interactive_shell_session(ssh_session session) {
    ssh_channel channel;
    int rc;
    char buffer[256];
    fd_set fds; // ファイルディスクリプタの集合を管理するための変数を追加
    int nbytes;

    // 以降のコードは、前のコードと同じです

    // SSHチャンネルが開いている間、以下の処理を繰り返す
    while (ssh_channel_is_open(channel) && !ssh_channel_is_eof(channel)) {
        FD_ZERO(&fds); // fdsを初期化
        FD_SET(0, &fds); // 標準入力のファイルディスクリプタを追加
        FD_SET(ssh_get_fd(session), &fds); // SSHセッションのファイルディスクリプタを追加

        // select関数を使って入力と出力を監視
        if (select(ssh_get_fd(session) + 1, &fds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        // 標準入力からデータが利用可能な場合、読み込んでSSHチャンネルに書き込む
        if (FD_ISSET(0, &fds)) {
            nbytes = read(0, buffer, sizeof(buffer));
            if (nbytes > 0) {
                ssh_channel_write(channel, buffer, nbytes);
            } else if (nbytes < 0) {
                perror("read");
                break;
            } else {
                break;
            }
        }

        // SSHセッションからデータが利用可能な場合、読み込んで標準出力に書き込む
        if (FD_ISSET(ssh_get_fd(session), &fds)) {
            nbytes = ssh_channel_read_timeout(channel, buffer, sizeof(buffer), 0, 100);
            if (nbytes > 0) {
                fwrite(buffer, 1, nbytes, stdout);
                fflush(stdout);
            } else if (nbytes < 0) {
                fprintf(stderr, "Error reading from channel: %s\n", ssh_get_error(channel));
                break;
            }
        }
    }
    
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);

    return SSH_OK;
}

int main(int argc, char *argv[])
{
    ssh_session session;
    int rc;
    char *password;

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <hostname> <username> <password>\n", argv[0]);
        exit(1);
    }

    session = ssh_new();
    if (session == NULL) {
        fprintf(stderr, "Error creating SSH session.\n");
        exit(1);
    }

    ssh_options_set(session, SSH_OPTIONS_HOST, argv[1]);
    ssh_options_set(session, SSH_OPTIONS_USER, argv[2]);
    password = argv[3];

    rc = ssh_connect(session);
    if (rc != SSH_OK) {
        fprintf(stderr, "Error connecting to SSH server: %s\n", ssh_get_error(session));
        ssh_free(session);
        exit(1);
    }

    rc = ssh_userauth_password(session, NULL, password);
    if (rc != SSH_AUTH_SUCCESS) {
        fprintf(stderr, "Error authenticating with password: %s\n", ssh_get_error(session));
        ssh_disconnect(session);
        ssh_free(session);
        exit(1);
    }

    printf("Authenticated successfully!\n");

    // インタラクティブシェルセッションを開始
    if (interactive_shell_session(session) != SSH_OK) {
        fprintf(stderr, "Error in interactive shell session.\n");
    }

    ssh_disconnect(session);
    ssh_free(session);

    return 0;
}
