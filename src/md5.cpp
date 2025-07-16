// modified by:王博宇
//  MD5加密函数实现
//  该函数使用OpenSSL库来计算字符串的MD5哈希值
//  确保在编译时链接OpenSSL库，例如使用 -lssl -lcrypto 选项
//  注意：在实际项目中，请确保使用安全的MD5实现
//        这里的实现仅用于演示目的，实际项目中请使用成熟的库。
#include "md5.h"
#include <iomanip>
#include <sstream>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <vector>
#include <iostream>
// modified by:王博宇
string md5(const string &input)
{
    // 创建 EVP_MD_CTX 上下文对象，用于消息摘要计算
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (ctx == nullptr)
    {
        // 处理创建失败情况: 简单返回空字符串
        return "";
    }

    // 初始化 MD5 算法的消息摘要操作
    const EVP_MD *md = EVP_md5();
    if (EVP_DigestInit_ex(ctx, md, nullptr) != 1)
    {
        EVP_MD_CTX_free(ctx);
        return "";
    }

    // 传入数据进行哈希计算
    if (EVP_DigestUpdate(ctx, input.c_str(), input.length()) != 1)
    {
        EVP_MD_CTX_free(ctx);
        return "";
    }

    // 改用 vector 动态分配内存
    vector<unsigned char> digest(EVP_MD_size(md));
    unsigned int digest_len;

    if (EVP_DigestFinal_ex(ctx, digest.data(), &digest_len) != 1)
    {
        EVP_MD_CTX_free(ctx);
        return "";
    }

    // 释放上下文对象
    EVP_MD_CTX_free(ctx);

    // 将二进制哈希结果转换为十六进制字符串
    ostringstream oss;
    oss << hex << setfill('0');
    for (unsigned int i = 0; i < digest_len; ++i)
    {
        oss << setw(2) << static_cast<int>(digest[i]);
    }

    return oss.str();
}
