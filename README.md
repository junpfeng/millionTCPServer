# millionTCPServer
![language](https://img.shields.io/badge/C++-11-green.svg )![platform](https://img.shields.io/badge/platform-Linux/Windows-blue.svg )![complier](https://img.shields.io/badge/complier-gcc7.3.1-red.svg )![status](https://img.shields.io/badge/status-updating-yellow.svg )    

## Introduce

The purpose of the project is to build a `TCP` server that supports millions of connections. The project's feature is that it does not use any third-party network library, but uses the system's own `select` 、`epoll`or`IOCP` models to build the server and provide relatively complete Supporting functions. The project will be continuously updated.

## Usage

### Linux

* **configure file**

  *updating*

* **start the server side**

  Enter the` HelloSocket / EasyTcpServer `directory, execute the `make` command to compile and generate the executable file on the server side, execute the `./server` command to run the server.

* **start the client side**

  Enter the`HelloSocket / EasyTcpClient`directory, execute the `make` command to compile and generate the executable file on the server side, execute the `./client` command to run the client.

### Windows

* **configure file**

  updating

* **others**

  In the `Windows` environment, it is recommended to use the relevant `IDE` and just add the source file, so I won't go into details

## Structure

![structure_1](https://github.com/junpfeng/millionTCPServer/blob/master/image/Architecture_1.png)

## Feature

* `IO multiplexing`
* `Application layer data protocol`
* `Thread Pool`
* `Thread Sync`
* `Multitask separation`
* `Memory management`
* `Heartbeat detection`
* `Async send and receive`
* `Stream data`

## Future work

*updating*
