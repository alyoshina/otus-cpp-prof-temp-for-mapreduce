#pragma once

#include "utils.h"

#include <iostream>

//#include "libssh2_setup.h"
#include <libssh2.h>
#include <libssh2_sftp.h>

#include <boost/asio/thread_pool.hpp>
#include <boost/asio/signal_set.hpp>



#include <format>

class LibSSH2 {
public:
  static void Init() {
     static LibSSH2 instance;
  }
private:
  explicit LibSSH2() {
     if (libssh2_init(0) != 0) {
        ;
        std::cout << "ERROR libssh2_init" << std::endl;
        //throw std::runtime_error("libssh2 initialization failed");
     }
  }
  ~LibSSH2() {
     std::cout << "shutdown libssh2" << std::endl;
     libssh2_exit();
  }
};

class Session {
public:
  explicit Session(/*const bool enable_compression*/) : m_session(libssh2_session_init()) {
     if (m_session == nullptr) {
        throw std::runtime_error("failed to create libssh2 session");
     } else {
        std::cout << "libssh2_session_init ok" << std::endl;
     }

     //libssh2_session_set_blocking(m_session, 0);
    //  if (enable_compression) {
    //     libssh2_session_flag(m_session, LIBSSH2_FLAG_COMPRESS, 1);
    //  }
  }

  ~Session() {
     const std::string desc = "Shutting down libssh2 session";
     libssh2_session_disconnect(m_session, desc.c_str());
     libssh2_session_free(m_session);
  }

//private:
  LIBSSH2_SESSION *m_session;
};

class CreateConnection {
public:
    static CreateConnection& init() {
        static CreateConnection instance;
        return instance;
    }
    template<typename T, typename Client>
    ba::awaitable<std::shared_ptr<T>> connection(std::shared_ptr<Client> client) {
        auto initiate = [this, client]<typename Handler>(Handler&& handler) mutable {
            ba::post(threadPool //client->getParserContext().get_executor()
                        , [handler = std::forward<Handler>(handler), this, client]() mutable {
                std::string ip = client->getIp();
                handler(this->connect<T>(std::move(ip)));
            });
        };
        return ba::async_initiate<decltype(ba::use_awaitable), void(std::shared_ptr<T>)>(initiate, ba::use_awaitable);
    }
    ba::thread_pool& getThreadPool() { return threadPool; }
private:
    template<typename T>
    std::shared_ptr<T> connect(std::string &&ip) {
        return std::make_shared<T>(ip);
    }

    explicit CreateConnection()
    : threadPool(1)
    , signals({threadPool, SIGINT, SIGTERM})
    {
        signals.async_wait([this](auto, auto) { std::cout << "threadPool.stop" << std::endl; threadPool.stop(); });
    }
    ~CreateConnection() {
        threadPool.join();
    }
    ba::thread_pool threadPool;
    ba::signal_set signals;
};

class SshConnection {
public:
    SshConnection(std::string &ip) {
        LibSSH2::Init();
        uint32_t hostaddr = inet_addr(ip.c_str());
        //connect socket
        struct sockaddr_in sin;
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if(sock == LIBSSH2_INVALID_SOCKET) {
            std::cout << "failed to create socket" << std::endl;
            // fprintf(stderr, "failed to create socket.\n");
            // goto shutdown;
        } else {
            std::cout << "ok to create socket" << std::endl;
        }
        sin.sin_family = AF_INET;
        sin.sin_port = htons(22);
        sin.sin_addr.s_addr = hostaddr;
        //memcpy(&sin.sin_addr, ip.c_str(), ip.length());
        //sin.sin_addr.s_addr = htonl(0x7F000001);
        if(connect(sock, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in))) {
            std::cout << "failed to connect" << std::endl;
            // fprintf(stderr, "failed to connect.\n");
            // goto shutdown;
        } else {
            std::cout << "ok to connect" << std::endl;
        }
        //hand shake
        int rc = libssh2_session_handshake(session.m_session, sock);
        if(rc) {
            std::cout << "Failure establishing SSH session " << rc << std::endl;
            // fprintf(stderr, "Failure establishing SSH session: %d\n", rc);
            // goto shutdown;
        }
        libssh2_session_set_blocking(session.m_session, 1);

        //process known hosts
        const char *fingerprint = libssh2_hostkey_hash(session.m_session, LIBSSH2_HOSTKEY_HASH_SHA1 );
        std::string str("Fingerprint: ");
        for(int i = 0; i < 20; i++) {
            str += std::format("{:X} ", (unsigned char)fingerprint[i]);
        }
        std::cout << str << std::endl;
        //auth
        bool auth_pw = true;
        const char *username = "dts";
        const char *password = "dts";
        const char *sftppath = "/home/dts/testssh/file.txt";
        if(auth_pw) {
            /* We could authenticate via password */ 
            while((rc = libssh2_userauth_password(session.m_session, username, password)) == LIBSSH2_ERROR_EAGAIN);
            if(rc) {
                std::cout << "Authentication by password failed " << rc << std::endl;
                // fprintf(stderr, "Authentication by password failed.\n");
                // goto shutdown;
            }
        } else {
            /* Or by public key */ 
            // while((rc = libssh2_userauth_publickey_fromfile(session, username,
            //                                         pubkey, privkey,
            //                                         password)) == LIBSSH2_ERROR_EAGAIN);
            // if(rc) {
            //     std::cout << "Authentication by public key failed" << std::endl;
            //     // fprintf(stderr, "Authentication by public key failed.\n");
            //     // goto shutdown;
            // }
        }

        sftp_session = libssh2_sftp_init(session.m_session);
        if(!sftp_session) {
            std::cout << "Unable to init SFTP session" << std::endl;
            // fprintf(stderr, "Unable to init SFTP session\n");
            // goto shutdown;
        }
        /* Request a file via SFTP */ 
        sftp_handle = libssh2_sftp_open(sftp_session, sftppath,
                                    LIBSSH2_FXF_READ, 0);
        //std::cout << "getSize=" << getSize() << std::endl;


    }
    ~SshConnection() {
        libssh2_sftp_close(sftp_handle);
        libssh2_sftp_shutdown(sftp_session);

        if(sock != LIBSSH2_INVALID_SOCKET) {
            shutdown(sock, 2);
            close(sock);
        }
    }
    int getSize() {
        LIBSSH2_SFTP_ATTRIBUTES attrs;
        int ret = libssh2_sftp_fstat_ex(sftp_handle, &attrs, 0);
        if (ret != 0) {
            ;
        }
        return attrs.filesize;
    }
    void seek(std::size_t offset) {
        libssh2_sftp_seek(sftp_handle, offset);
    }
    std::size_t tell() {
        return libssh2_sftp_tell(sftp_handle);
    }
    bool getline(std::string &line, const char ch = '\n') {
        line.clear();
        size_t pos = libssh2_sftp_tell(sftp_handle);
        do {
            char mem[10];
            ssize_t nread;    
            nread = libssh2_sftp_read(sftp_handle, mem, sizeof(mem));
            if (nread > 0) {
                for (ssize_t i = 0; i < nread; i++) {
                    if (mem[i] == ch) {
                        line += std::string(mem, i + 1);
                        seek(pos + i + 1);
                        return true;
                    }
                }
                line += std::string(mem, nread);
                pos += nread;
                seek(pos);
            } else {
                break;
            }
        } while(1);
        return false;//line.size();
    }
    void read() {
        do {
            char mem[1024];
            ssize_t nread;
    
            /* loop until we fail */ 
            nread = libssh2_sftp_read(sftp_handle, mem, sizeof(mem));

            if(nread > 0) {
                //write(1, mem, nread);
                std::cout << std::string(mem, nread) << std::endl;
            }
            else {
                break;
            }
        } while(1);
    }

    Session session;
    LIBSSH2_SFTP *sftp_session;
    LIBSSH2_SFTP_HANDLE *sftp_handle;
    libssh2_socket_t sock;
};