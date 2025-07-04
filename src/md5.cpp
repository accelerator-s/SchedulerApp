//modified by:王博宇
// MD5加密函数实现
// 该函数使用OpenSSL库来计算字符串的MD5哈希值
// 确保在编译时链接OpenSSL库，例如使用 -lssl -lcrypto 选项
// 注意：在实际项目中，请确保使用安全的MD5实现
//       这里的实现仅用于演示目的，实际项目中请使用成熟的库。
#include "md5.h"
#include<iomanip>
#include <sstream>
#include <openssl/md5.h>
#include <string>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <vector>
#include <iostream>
using namespace std;
//modified by:王博宇
string md5(const string& input) {
    // 创建 EVP_MD_CTX 上下文对象，用于消息摘要计算
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (ctx == nullptr) {
        // 处理创建失败情况，这里简单返回空字符串，实际可更完善
        return "";}

        // 初始化 MD5 算法的消息摘要操作
    const EVP_MD* md = EVP_md5();
    if (EVP_DigestInit_ex(ctx, md, nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }

    // 传入数据进行哈希计算
    if (EVP_DigestUpdate(ctx, input.c_str(), input.length()) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }

    // 改用 std::vector 动态分配内存
    vector<unsigned char> digest(EVP_MD_size(md));  
    unsigned int digest_len;

    if (EVP_DigestFinal_ex(ctx, digest.data(), &digest_len) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }
    

    // 释放上下文对象
    EVP_MD_CTX_free(ctx);

    // 将二进制哈希结果转换为十六进制字符串
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < digest_len; ++i) {
        oss << std::setw(2) << static_cast<int>(digest[i]);
    }

    return oss.str();
}
 
/**
 * @brief MD5加密占位符函数
 * 
 * @param input 原始字符串
 * @return std::string 加密后的字符串（伪）
 * @note 这是一个为了让项目能编译而设置的占位符。
 *       在实际项目中，您需要引入一个真正的MD5库并在这里实现它。
 *       例如： #include <openssl/md5.h> 并调用相应的函数。
 */
/*string md5(const string& input) {
    // 这是一个非常不安全的伪实现，仅用于演示
    return "md5_hash_of_" + input;
}
    modified by wby*/
