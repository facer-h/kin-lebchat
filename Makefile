# 编译器
CXX := g++
# 编译选项
CXXFLAGS := -std=c++17 -Wall -Wextra -Iserver_header
# 链接选项
LDLIBS := -lmysqlclient

# 优先使用 mysql_config 提供的头文件和链接参数
MYSQL_CFLAGS := $(shell mysql_config --cflags 2>/dev/null)
MYSQL_LIBS := $(shell mysql_config --libs 2>/dev/null)
ifneq ($(MYSQL_LIBS),)
  CXXFLAGS += $(MYSQL_CFLAGS)
  LDLIBS := $(MYSQL_LIBS)
endif

# 源文件列表
SRCS := src/manager_sql.cc \
        src/manage_user.cc \
        src/manage_user_friend.cc \
        src/manage_conversation.cc \
        src/manage_conversation_member.cc \
        src/manage_message.cc \
        src/main.cc

# 目标文件
OBJS := $(SRCS:.cc=.o)
# 可执行文件名
TARGET := kinchat_server

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDLIBS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
