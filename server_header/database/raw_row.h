#ifndef KINCHAT_HEADER_DATABASE_RAW_ROW_H_
#define KINCHAT_HEADER_DATABASE_RAW_ROW_H_

#include <map>
#include <string>

// 一行数据库记录：键为字段名，值为字段内容。
using RawRow = std::map<std::string, std::string>;

#endif  // KINCHAT_HEADER_DATABASE_RAW_ROW_H_
