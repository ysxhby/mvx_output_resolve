从数据采集到数据传输到数据解析已接能够调通


另外还增加了MissCout 对丢失的CALL和RET进行了计数


删除写入本地文件部分的代码

这个模块卸载不了，强制关机重启后修复文件系统是个问题


注释了vmxice_extract.c 180行的printk


修改m_buffer.h中定义的追踪数据格式，对每条数据加入了计数unsigned long serial_no和偶校验int even_parity; 