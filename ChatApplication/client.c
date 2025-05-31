
#include <stdio.h> //girdi çıktı işleri işte
#include <stdlib.h>
#include <string.h>
#include <unistd.h> //close falan bunda
#include <sys/socket.h> //socket connect işleri
#include <arpa/inet.h> //ıp adres işleri
#include <pthread.h> // thread için

#define PORT 8888

int client_socket;

// bu thread gelen mesajları yakalıyo
void *mesaj_alici(void *arg) {
    char buffer[1024];
    int len;
    while ((len = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        buffer[len] = '\0'; // sona \0 koyuyoz düzgün bitsin diye
        printf("%s\n", buffer); // gelen mesajı yazdır
    }
    return NULL; // bitti çık
}

int main() {
    struct sockaddr_in server_addr;
    char buffer[1024];
    char nickname[32];
    pthread_t alici_thread;

    // kullanıcıdan nick istiyoz
    printf("Nick gir: ");
    fgets(nickname, sizeof(nickname), stdin);
    nickname[strcspn(nickname, "\n")] = 0; // enter'ı sil

    // soket açıyoz
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket oluşturulamadı"); // hata olursa çıkarız
        return 1;
    }

    // sunucu bilgilerini ayarla, local ip işte
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // bağlan sunucuya
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bağlanılamadı");
        return 1;
    }

    // ilk nicki gönder sunucuya, kim olduğunu bilsin
    send(client_socket, nickname, strlen(nickname), 0);

    // alıcı thread başlat, gelen mesajlar bu işle dinlenecek
    if (pthread_create(&alici_thread, NULL, mesaj_alici, NULL) != 0) {
        perror("Thread başlatılamadı");
        return 1;
    }

    printf("Komutlar için /yardım yazabilirsin. Mesaj yaz (Ctrl+C ya da /quit ile çık):\n");

    while (1) {
        fgets(buffer, sizeof(buffer), stdin); // mesaj yaz
        buffer[strcspn(buffer, "\n")] = 0; // enter'ı sil

        if (strlen(buffer) == 0)
            continue; // boşsa geç

        if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
            perror("Mesaj gönderilemedi");
            break;
        }

        if (strncmp(buffer, "/", 1) != 0) {
            printf("[ben]: %s\n", buffer); // kendi mesajını göster
        }

        if (strcmp(buffer, "/quit") == 0)
            break; // çık komutuysa çık
    }

    close(client_socket); // işi bitti kapat
    return 0;
}
