  #include <libssh/libssh.h>
  #include <stdlib.h>
  #include <stdio.h>
  #include <errno.h>
  #include <string.h>

  // 対話的なシェルセッションを行う関数
  int interactive_shell_session(ssh_session session) {
      ssh_channel channel;
      int rc;
      char buffer[256];
      int nbytes;

      // SSHチャンネルを作成
      channel = ssh_channel_new(session);
      if (channel == NULL) {
          fprintf(stderr, "SSHチャンネルの作成に失敗しました。\n");
          return SSH_ERROR;
      }

      // SSHチャンネルを開く
      rc = ssh_channel_open_session(channel);
      if (rc != SSH_OK) {
          fprintf(stderr, "SSHチャンネルを開くのに失敗しました: %s\n", ssh_get_error(channel));
          ssh_channel_free(channel);
          return rc;
      }

      // PTY（擬似端末）を要求する
      rc = ssh_channel_request_pty(channel);
      if (rc != SSH_OK) {
          fprintf(stderr, "PTYの要求に失敗しました: %s\n", ssh_get_error(channel));
          ssh_channel_close(channel);
          ssh_channel_free(channel);
          return rc;
      }

      // シェルを要求する
      rc = ssh_channel_request_shell(channel);
      if (rc != SSH_OK) {
          fprintf(stderr, "シェルの要求に失敗しました: %s\n", ssh_get_error(channel));
          ssh_channel_close(channel);
          ssh_channel_free(channel);
          return rc;
      }

      // チャンネルが開いていて、EOF（End of File）でない間、データを読み込む
      while (ssh_channel_is_open(channel) && !ssh_channel_is_eof(channel)) {
          nbytes = ssh_channel_read_timeout(channel, buffer, sizeof(buffer), 0, 100);
          if (nbytes > 0) {
              fwrite(buffer, 1, nbytes, stdout);
              fflush(stdout);
          } else if (nbytes < 0) {
              fprintf(stderr, "チャンネルからの読み取りエラー: %s\n", ssh_get_error(channel));
              break;
          }

          if (feof(stdin)) {
              break;
          }
      }

      // チャンネルを閉じる
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

      // コマンドライン引数が足りない場合はエラーメッセージを表示
      if (argc < 4) {
          fprintf(stderr, "使い方: %s <ホスト名> <ユーザー名> <パスワード>\n", argv[0]);
          exit(1);
      }

      // SSHセッションを作成
      session = ssh_new();
      if (session == NULL) {
        fprintf(stderr, "SSHセッションの作成に失敗しました。\n");
        exit(1);
    }
  
    // オプションを設定（ホスト名、ユーザー名）
    ssh_options_set(session, SSH_OPTIONS_HOST, argv[1]);
    ssh_options_set(session, SSH_OPTIONS_USER, argv[2]);
    password = argv[3];
  
    // SSHサーバーへ接続
    rc = ssh_connect(session);
    if (rc != SSH_OK) {
        fprintf(stderr, "SSHサーバーへの接続に失敗しました: %s\n", ssh_get_error(session));
        ssh_free(session);
        exit(1);
    }
  
    // パスワードを使って認証
    rc = ssh_userauth_password(session, NULL, password);
    if (rc != SSH_AUTH_SUCCESS) {
        fprintf(stderr, "パスワード認証に失敗しました: %s\n", ssh_get_error(session));
        ssh_disconnect(session);
        ssh_free(session);
        exit(1);
    }
  
    printf("認証に成功しました！\n");
  
    // 対話的なシェルセッションを開始
    if (interactive_shell_session(session) != SSH_OK) {
        fprintf(stderr, "対話的シェルセッションでエラーが発生しました。\n");
    }
  
    // SSHセッションを切断して解放
    ssh_disconnect(session);
    ssh_free(session);
  
    return 0;
  }  
