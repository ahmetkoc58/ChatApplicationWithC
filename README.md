
# 📡 Multi-Client Chat Server with C and POSIX Threads

This is a simple multi-client chat server project implemented in **C** using **POSIX sockets and pthreads**. It allows multiple clients to connect to a central server, exchange public and private messages, and interact using basic chat commands. The project was developed in a Linux-like environment (Cygwin) as part of a systems programming assignment.

## ✨ Features

- 🧵 Threaded client handling with `pthread_create()`
- 💬 Public (broadcast) and private (`@username`) messaging
- ✅ Nickname system with duplication check
- 📋 `/list` command shows connected users
- ❓ `/help` command lists available commands
- 🚪 `/quit` command to leave the chat
- 📝 Logs all messages and connections to `chat_server.log`
- 👋 Server notifies everyone when users join or leave

## ⚙️ Technologies Used

- C Programming Language  
- POSIX Sockets  
- Pthreads (`pthread.h`)  
- Cygwin (UNIX-like environment on Windows)

## 🏁 How to Run

### 1. Compile

```bash
gcc -o server server.c -lpthread
gcc -o client client.c -lpthread
```

### 2. Run the Server

```bash
./server
```

### 3. Run the Client (in separate terminals)

```bash
./client
```

## 📝 Notes

- Each client must enter a **unique nickname** upon connection.
- The log file `chat_server.log` will be created in the same directory.
- Ensure ports are not blocked by firewalls if testing over network.

Enjoy chatting! 🎉
