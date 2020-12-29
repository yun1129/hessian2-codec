#include <iostream>

#include "hessian2/codec.hpp"
#include "hessian2/basic_codec/object_codec.hpp"

int main() {
    std::string out;
    ::hessian2::Encoder encode(out);
    encode.encode<std::string>("test string");
    ::hessian2::Decoder decode(out);
    auto ret = decode.decode<std::string>();
    if (ret) {
        std::cout << *ret << std::endl;
    } else {
        std::cerr << "decode failed: " << decode.getErrorMessage() << std::endl;
    }
    return 0;
}