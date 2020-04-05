# millionTCPServer
## 分支结构
* `master` 分支发布稳定版本
* `dev_Linux` 分支用于Linux环境下的开发
* `dev_win` 分支用于windows环境下的开发
* `dev` 分支用于合并同时兼任Linux和windows的版本
## 目录和文件结构
* 主要包含两个目录：
* `EasyTcpServer` 是服务端源文件目录
* `EasyTcpClient` 是客户端源文件目录
* 其他同层的目录是一些测试目录，可以忽略
## 不同环境的支持
* 需要支持 `c++11` 及以上的版本
* `windows` 环境下，使用 `VS` 建立好工程后，将源文件添加进去
* `Linux` 环境，使用 `makefile` 进行编译
