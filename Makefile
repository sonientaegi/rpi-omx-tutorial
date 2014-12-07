# Simple makefile for rpi-openmax-demos.

PROGRAMS = 	buffer_allocate buffer_use camera_tunnel camera_tunnel_non camera_render camera_render_fps camera_encode
# load_component buffer_allocate buffer_use execute_non-tunnelling execute_non-tunnelling_cam yuv_dump execute_tunnelling
OBJS	 =	common.o OMXsonien.o
CC	 	 = 	gcc
CFLAGS	 =	-DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
			-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX \
			-DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM \
			-I/opt/vc/include -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux \
		   	-fPIC -ftree-vectorize -pipe -Wall -O2 -g -std=gnu99
LDFLAGS	 = 	-L/opt/vc/lib -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lm -lcurses

# Define whather using CURSES or not. If you want to use CURSES please uncomment below line.
# CFLAGS	+=	-DCURSES

# all 은 OBJS 와 PROGRAMS 에 종속된다 
all : $(OBJS) $(PROGRAMS)

# 모든 프로그램은 $(OBJS) 를 참조한다. 
$(PROGRAMS) : $(OBJS) 

# 명시하지는 않았지만, $(OBJS) 를 생성하는 컴파일이 수행된다. 링크는 진행되지 않는다.
# 만약 링크가 진행되면 Main 이 없어서 오류가 발생한다. 

# Project clean or clear 시 모든 프로그램과 오브젝트를 삭제한다.
clean clear : 
	rm -f $(PROGRAMS) $(OBJS)
	
.PHONY: all clean
