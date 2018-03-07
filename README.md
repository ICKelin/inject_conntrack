## 目的
在实际应用当中，有些应用层希望能够查询到内核保存的conntrack记录，项目提供一种解决这一问题的方案以供参考。

## 思路
在应用层数据和传输层之间插入协议，该协议包含有conntrack记录，需要修改用户层读取数据的部分，先将inject_conntrack.ko插入的协议进行解码，然后再进行数据读取。

## 运行
Linux环境下

```
# 安装模块
make && make install

# 卸载模块
make uninstall

```