# 此文件由makeobj.sh自动生成
############################################################
# 项目名 : 
# 模块名 : 
# 备  注 : 
############################################################

###### 源文件配置区
#@ c_FILE
c_FILE		=	\
			memque.c \
			HashX.c \
			LOGC.c \
			libsimspider.c \
			libsimspider-queue.c \

c_FILE2		=	\
			simspider.c \

###### 目标文件、安装目录配置区
LIB		=	libsimspider.so
LIBINST		=	/usr/local/lib
BIN		=	simspider
BININST		=	/usr/local/bin
HDER		=	libsimspider.h
HDERINST	=	/usr/local/include/simspider

###### 编译选项
CFLAGS		=	-I. \
			-I/usr/local/include \

###### 链接选项
LFLAGS		=	-L. \
			-L/usr/lib \
			-lcurl \

###### 额外宏定义区
CLEAN_ADDITION	=	simspider.log

###### 加载mktpl模板库
#@ make_all
#@ make_clean
#@ make_install
#@ make_install_BININST
#@ make_install_LIBINST
#@ make_install_HDERINST

include $(MKTPLDIR)/makeobj_$(MKTPLOS).inc

###### 目标文件依赖关系
libsimspider.so		:	$(c_FILE_o)
	$(CC) $(SOFLAGS) -o $@ $(c_FILE_o) $(LFLAGS)

simspider		:	$(c_FILE2_o)
	$(CC) $(ESOFLAGS) -o $@ $(c_FILE2_o) $(LFLAGS) -lsimspider

