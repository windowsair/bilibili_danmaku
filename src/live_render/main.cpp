#include "live_danmaku.h"
#include <iostream>

#include "thirdparty/openssl/x64-windows/include/openssl/crypto.h"

#include "thirdparty/IXWebSocket/ixwebsocket/IXNetSystem.h"
#include "thirdparty/IXWebSocket/ixwebsocket/IXWebSocket.h"
#include "thirdparty/IXWebSocket/ixwebsocket/IXUserAgent.h"



int main() {
    ix::initNetSystem();


    OPENSSL_init();
    return -1;
}
