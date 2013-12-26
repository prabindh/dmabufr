#ifndef DMABUFR_H
#define DMABUFR_H

#define FOURCC_FMT(f,a,b,c) \
    ((unsigned long) ((f) | (a)<<8 | (b)<<16 | (c)<<24))

/* RGB */
#define FMT_RGB565   FOURCC_FMT('R', 'G', 'B', 'P')
#define FMT_ARGB     FOURCC_FMT('A', 'R', 'G', 'B')
/* PLANAR YUV */
#define FMT_I420     FOURCC_FMT('I', '4', '2', '0')
#define FMT_YV12     FOURCC_FMT('Y', 'V', '1', '2')
#define FMT_NV12     FOURCC_FMT('N', 'V', '1', '2')
/* INTERLEAVED YUV */
#define FMT_UYVY     FOURCC_FMT('U', 'Y', 'V', 'Y')
#define FMT_YUYV     FOURCC_FMT('Y', 'U', 'Y', 'V')

typedef struct _DMABUFR_ALLOCATION_INFO
{
	unsigned int width;
	unsigned int height;
	unsigned int format;
}DMABUFR_ALLOCATION_INFO;

typedef struct _DMABUFR_IO
{
	void* input;
	void* output;
}DMABUFR_IO;

/* Be content with g */
#define DMABUFR_GID                  'g'
/* Both read and write ioctls */
#define DMABUFR_IOWR(INDEX)          _IOWR(DMABUFR_GID, INDEX, DMABUFR_IO)

#define DMABUFR_IOCTL_REQUEST_FD	DMABUFR_IOWR(0)
#define DMABUFR_IOCTL_CONNECT_FD	DMABUFR_IOWR(1)
#define DMABUFR_IOCTL_ATTACH_FD		DMABUFR_IOWR(2)
#define DMABUFR_IOCTL_USE_BUFFER_FD		DMABUFR_IOWR(3)
#define DMABUFR_IOCTL_END_OF_OPERATION_FD		DMABUFR_IOWR(4)
#define DMABUFR_IOCTL_DETACH_FD			DMABUFR_IOWR(5)

#endif /* DMABUFR_H */
