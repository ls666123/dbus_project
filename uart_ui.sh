#!/bin/bash
#
# uart_ui.sh - Dialog Shell 脚本绘制 UART UI
#
# 功能:
#   1. 从用户态输入数据给 Socket 客户端
#   2. 查询 database 数据
#
# 使用: ./uart_ui.sh [server_ip]
# 依赖: dialog, nc (netcat)
#

SERVER_IP="${1:-127.0.0.1}"
SERVER_PORT=8888
TITLE="Kangaroo's UART UI"

# 临时文件
TMPFILE=$(mktemp /tmp/uart_ui.XXXXXX)
trap "rm -f $TMPFILE" EXIT

#
# 发送数据到 socket server 并接收响应
# 参数: $1 = 要发送的数据
# 返回: 服务器响应保存在 TMPFILE
#
send_to_server() {
    local data="$1"
    local response

    # 使用 nc 发送数据并接收响应（超时 3 秒）
    response=$(echo -n "$data" | nc -w 3 "$SERVER_IP" "$SERVER_PORT" 2>/dev/null)

    if [ $? -ne 0 ] || [ -z "$response" ]; then
        echo "Error: Cannot connect to server $SERVER_IP:$SERVER_PORT" > "$TMPFILE"
        return 1
    fi

    echo "$response" > "$TMPFILE"
    return 0
}

#
# 功能1: 发送数据到 Server
#
send_data_menu() {
    # 弹出输入框让用户输入数据
    dialog --title "Send Data to Server" \
           --backtitle "$TITLE" \
           --inputbox "Please enter data to send to server:" \
           10 60 2>"$TMPFILE"

    local ret=$?

    # 用户按了 Cancel
    if [ $ret -ne 0 ]; then
        return
    fi

    local input
    input=$(cat "$TMPFILE")

    # 空输入检查
    if [ -z "$input" ]; then
        dialog --title "Warning" \
               --backtitle "$TITLE" \
               --msgbox "Input cannot be empty!" 6 40
        return
    fi

    # 发送到服务器
    send_to_server "$input"

    if [ $? -eq 0 ]; then
        local response
        response=$(cat "$TMPFILE")
        dialog --title "Server Response" \
               --backtitle "$TITLE" \
               --msgbox "Sent: $input\n\nResponse:\n$response" 12 60
    else
        local error
        error=$(cat "$TMPFILE")
        dialog --title "Error" \
               --backtitle "$TITLE" \
               --msgbox "$error" 8 50
    fi
}

#
# 功能2: 查询 Database 数据
#
query_database_menu() {
    # 发送查询命令
    send_to_server "CMD:QUERY"

    if [ $? -eq 0 ]; then
        local result
        result=$(cat "$TMPFILE")

        # 使用 msgbox 显示查询结果（支持滚动）
        dialog --title "Database Query Results" \
               --backtitle "$TITLE" \
               --msgbox "$result" 20 70
    else
        local error
        error=$(cat "$TMPFILE")
        dialog --title "Error" \
               --backtitle "$TITLE" \
               --msgbox "$error\n\nPlease make sure the server is running." 10 50
    fi
}

#
# 主菜单循环
#
main_menu() {
    while true; do
        dialog --title "Menu" \
               --backtitle "$TITLE" \
               --cancel-label "Exit" \
               --menu "Please select:" 15 50 4 \
               1 "Send data to server" \
               2 "Query database" \
               2>"$TMPFILE"

        local ret=$?

        # 用户按了 Exit
        if [ $ret -ne 0 ]; then
            break
        fi

        local choice
        choice=$(cat "$TMPFILE")

        case "$choice" in
            1) send_data_menu ;;
            2) query_database_menu ;;
        esac
    done
}

# ==================== 主程序 ====================

# 检查 dialog 是否安装
if ! command -v dialog &>/dev/null; then
    echo "Error: 'dialog' is not installed."
    echo "Install with: sudo apt install dialog"
    exit 1
fi

# 检查 nc 是否可用
if ! command -v nc &>/dev/null; then
    echo "Error: 'nc' (netcat) is not installed."
    echo "Install with: sudo apt install netcat-openbsd"
    exit 1
fi

# 启动主菜单
main_menu

# 清屏退出
clear
echo "UART UI exited. Goodbye!"
