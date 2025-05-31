
#include <stdio.h> // girdi çıktı işleri işte
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // close falan bunda
#include <pthread.h> // thread işleri
#include <sys/socket.h> // soket işleri
#include <arpa/inet.h> // ip adres işleri
#include <time.h> // saat tarih işleri

#define MAXCLIENT 30
#define PORT 8888

// her clientin bilgileri burda tutuluyo
struct client {
    int socket; // soketi
    struct sockaddr_in address; // ip falan
    pthread_t thread_id; // threadi
    char nickname[32]; // ismi
};

// client listesi ve kilit
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
struct client* clients[MAXCLIENT]; // bağlı olanlar burda

// log dosyasına yazıyo
void log_yaz(char *metin) {
    FILE *f = fopen("chat_server.log", "a");
    if (f) {
        time_t simdi = time(NULL);
        struct tm *t = localtime(&simdi);
        char zaman[64];
        strftime(zaman, sizeof(zaman), "%Y-%m-%d %H:%M:%S", t);
        fprintf(f, "[%s] %s\n", zaman, metin);
        fclose(f);
    }
}

// sunucudan herkese mesaj atıyoz
void mesaj_gonder_sunucu(char *mesaj) {
    pthread_mutex_lock(&clients_mutex);
    char giden[1024];
    sprintf(giden, "[server]: %s", mesaj);
    for (int i = 0; i < MAXCLIENT; i++) {
        if (clients[i]) {
            send(clients[i]->socket, giden, strlen(giden), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// gelen kişiyi listeye koy
void addClient(struct client *client) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAXCLIENT; ++i) {
        if (!clients[i]) {
            clients[i] = client;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// biri çıkınca listeden sil
void removeClient(int soket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAXCLIENT; ++i) {
        if (clients[i] && clients[i]->socket == soket) {
            clients[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// nick zaten kullanılıyor mu ona bakıyoz
int nickname_kullaniliyor_mu(char *name) {
    for (int i = 0; i < MAXCLIENT; i++) {
        if (clients[i] && strcmp(clients[i]->nickname, name) == 0) {
            return 1;
        }
    }
    return 0;
}

// /liste yazınca aktifleri yolluyo
void liste_gonder(int hedef_socket) {
    pthread_mutex_lock(&clients_mutex);
    char cevap[1024];
    strcpy(cevap, "Aktif kullanıcılar:\n");
    for (int i = 0; i < MAXCLIENT; i++) {
        if (clients[i]) {
            strcat(cevap, "- ");
            strcat(cevap, clients[i]->nickname);
            strcat(cevap, "\n");
        }
    }
    send(hedef_socket, cevap, strlen(cevap), 0);
    pthread_mutex_unlock(&clients_mutex);
}

// yardım komutu burdan
void yardim_gonder(int soket) {
    char yazi[] =
        "Komutlar:\n"
        "/liste  → kim var kim yok gösterir\n"
        "/quit   → çıkarsın\n"
        "/yardım → bu liste\n"
        "@kisi mesaj → ona özel mesaj\n";
    send(soket, yazi, strlen(yazi), 0);
}

// mesajı özel mi genel mi ona bakıyo
void mesaj_gonder(char *gonderen, char *mesaj) {
    pthread_mutex_lock(&clients_mutex);
    if (mesaj[0] == '@') { // özel mesajsa
        char hedef_nick[32];
        int i = 1, j = 0;
        while (mesaj[i] != ' ' && mesaj[i] != '\0' && j < 31) {
            hedef_nick[j++] = mesaj[i++];
        }
        hedef_nick[j] = '\0';
        char *gercek_mesaj = strchr(mesaj, ' ');
        if (gercek_mesaj) {
            gercek_mesaj++;
            for (int k = 0; k < MAXCLIENT; k++) {
                if (clients[k] && strcmp(clients[k]->nickname, hedef_nick) == 0) {
                    char giden[1024];
                    sprintf(giden, "[özel] %s: %s", gonderen, gercek_mesaj);
                    send(clients[k]->socket, giden, strlen(giden), 0);
                    char logg[1024];
                    sprintf(logg, "%s -> @%s: %s", gonderen, hedef_nick, gercek_mesaj);
                    log_yaz(logg);
                    pthread_mutex_unlock(&clients_mutex);
                    return;
                }
            }
        }
        // kişi yoksa hata mesajı
        for (int k = 0; k < MAXCLIENT; k++) {
            if (clients[k] && strcmp(clients[k]->nickname, gonderen) == 0) {
                char hata[] = "böyle bi kullanıcı yok\n";
                send(clients[k]->socket, hata, strlen(hata), 0);
                break;
            }
        }
    } else { // genel mesaj
        char giden[1024];
        sprintf(giden, "%s: %s", gonderen, mesaj);
        for (int i = 0; i < MAXCLIENT; i++) {
            if (clients[i] && strcmp(clients[i]->nickname, gonderen) != 0) {
                send(clients[i]->socket, giden, strlen(giden), 0);
            }
        }
        char logg[1024];
        sprintf(logg, "%s (broadcast): %s", gonderen, mesaj);
        log_yaz(logg);
    }
    pthread_mutex_unlock(&clients_mutex);
}

// burda her clientle tek tek ilgileniyo
void *handle_client(void *arg) {
    struct client *cli = (struct client *)arg;
    char buffer[1024], log_mesaj[128];

    // ilk nicki alıyoz
    int len = recv(cli->socket, cli->nickname, sizeof(cli->nickname), 0);
    if (len <= 0) {
        close(cli->socket);
        free(cli);
        pthread_exit(NULL);
    }
    cli->nickname[len] = '\0';

    // nick kullanılıyosa kes
    if (nickname_kullaniliyor_mu(cli->nickname)) {
        char *mesaj = "Bu isim zaten var. Bağlantı kesiliyor.\n";
        send(cli->socket, mesaj, strlen(mesaj), 0);
        close(cli->socket);
        free(cli);
        pthread_exit(NULL);
    }

    // ekle listeye
    addClient(cli);
    sprintf(log_mesaj, "Client connected: %s", cli->nickname);
    log_yaz(log_mesaj);
    char katildi[64];
    sprintf(katildi, "%s sohbete katıldı", cli->nickname);
    mesaj_gonder_sunucu(katildi);

    // döngü: mesaj alana kadar
    while (1) {
        len = recv(cli->socket, buffer, sizeof(buffer), 0);
        if (len <= 0) break;
        buffer[len] = '\0';

        if (strcmp(buffer, "/quit") == 0) break;
        else if (strcmp(buffer, "/liste") == 0) liste_gonder(cli->socket);
        else if (strcmp(buffer, "/yardım") == 0 || strcmp(buffer, "/yardim") == 0) yardim_gonder(cli->socket);
        else {
            printf("%s: %s\n", cli->nickname, buffer);  // ekrana bas
            mesaj_gonder(cli->nickname, buffer);
        }
    }

    // çıkış işlemleri
    close(cli->socket);
    removeClient(cli->socket);
    sprintf(log_mesaj, "Client disconnected: %s", cli->nickname);
    log_yaz(log_mesaj);
    char ayrildi[64];
    sprintf(ayrildi, "%s sohbetten ayrıldı", cli->nickname);
    mesaj_gonder_sunucu(ayrildi);
    free(cli);
    pthread_exit(NULL);
}

// ana fonksiyon burda başlıyo her şey
int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    memset(clients, 0, sizeof(clients)); // listeyi boşalt

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) { perror("Socket oluşturulamadı"); exit(EXIT_FAILURE); }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind başarısız"); exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAXCLIENT) < 0) {
        perror("Listen başarısız"); exit(EXIT_FAILURE);
    }

    printf("Sunucu %d portunda başlatıldı.\n", PORT);
    log_yaz("Server started");

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0) { perror("Bağlantı alınamadı"); continue; }

        struct client *new_client = (struct client *)malloc(sizeof(struct client));
        if (!new_client) {
            perror("malloc başarısız");
            close(client_socket);
            continue;
        }

        new_client->socket = client_socket;
        new_client->address = client_addr;

        if (pthread_create(&new_client->thread_id, NULL, handle_client, (void *)new_client) != 0) {
            perror("Thread oluşturulamadı");
            close(new_client->socket);
            free(new_client);
        }
    }

    close(server_socket);
    return 0;
}
