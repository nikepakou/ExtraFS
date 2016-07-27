#ifndef _FS_H
#define _FS_H

#include <sys/types.h>

#define NR_OPEN 20 //�ļ�ϵͳ���ɵ��ļ���
#define NR_INODE 32
#define NR_FILE 64
#define NR_SUPER 8

/*������ṹ*/
struct super_block {
	unsigned short s_ninodes;
	unsigned short s_nzones;
	unsigned short s_imap_blocks;
	unsigned short s_zmap_blocks;
	unsigned short s_firstdatazone;
	unsigned short s_log_zone_size;
	unsigned long s_max_size;
	unsigned short s_magic;
/* These are only in memory */
	struct buffer_head * s_imap[8];
	struct buffer_head * s_zmap[8];
	unsigned short s_dev;
	struct m_inode * s_isup;
	struct m_inode * s_imount;
	unsigned long s_time;
	struct task_struct * s_wait;
	unsigned char s_lock;
	unsigned char s_rd_only;
	unsigned char s_dirt;
};

/*�����ϵ������ڵ����ݽṹ*/
struct d_inode {
	unsigned short i_mode;
	unsigned short i_uid;
	unsigned long i_size;
	unsigned long i_time;
	unsigned char i_gid;
	unsigned char i_nlinks;
	unsigned short i_zone[9];
};

/*�ڴ�i�ڵ�ṹ*/
struct m_inode {
	unsigned short i_mode;
	unsigned short i_uid;
	unsigned long i_size;
	unsigned long i_mtime;
	unsigned char i_gid;
	unsigned char i_nlinks;
	unsigned short i_zone[9];
/* these are in memory also */
	struct task_struct * i_wait;
	unsigned long i_atime;
	unsigned long i_ctime;
	unsigned short i_dev;
	unsigned short i_num;
	unsigned short i_count;
	unsigned char i_lock;
	unsigned char i_dirt;
	unsigned char i_pipe;
	unsigned char i_mount;
	unsigned char i_seek;
	unsigned char i_update;
};
//�ļ��ṹ
struct file {
	unsigned short f_mode;
	unsigned short f_flags;
	unsigned short f_count;
	struct m_inode * f_inode;
	off_t f_pos;  //�ļ���ǰ��дָ��
};
//Ŀ¼�ṹ
struct dir_entry {
	unsigned short inode;  //������Ŀ¼���С
	char name[NAME_LEN];  //������Ŀ¼������
};

struct m_inode inode_table[NR_INODE];  //�洢����ʹ�õ��ļ�i�ڵ�
struct file file_table[NR_FILE];  //�洢���н��̴򿪵��ļ����ݽṹ
struct super_block super_block[NR_SUPER];

#endif