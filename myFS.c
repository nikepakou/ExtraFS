#include<unistd.h>
//#include<sys/type.h>
#include <fcntl.h>
#include<time.h>
#include<stdlib.h>

#define BSIZE 20
#define NUM 32  //空闲 i节点大小,即该文件系统容纳的文件数
#define INODEBSIZE 64 //inode表大小

//#define MODE_FILE 1 //mfcreate 创建方式
#define MODE_DIR 1
#define MODE_FILE 2

//超级块结构
struct filesys {
        unsigned int s_size;        //总大小                                          
        unsigned int s_itsize;        //inode表大小                                        
        unsigned int s_freeinodesize;    //空闲i节点的数量                          
        unsigned int s_nextfreeinode;    //下一个空闲i节点
        unsigned int s_freeinode[NUM];    //空闲i节点数组  0:表示不空闲，1:表示空闲
        unsigned int s_freeblocksize;    //空闲块的数量          
        unsigned int s_nextfreeblock;    //下一个空闲块
        unsigned int s_freeblock[NUM];    //空闲块数组
        unsigned int s_pad[];        //填充到512字节  
};
//磁盘inode结构
struct finode {
        int fi_mode;            //类型：文件/目录
        int fi_uid_unused;        //uid，由于和进程无关联，仅仅是模拟一个FS，未使用，下同
        int fi_gid_unused;
        int fi_nlink;            //链接数，当链接数为0，意味着被删除
        long int fi_size;        //文件大小
        long int fi_addr[BNUM];        //文件块一级指针，并未实现多级指针
        time_t  fi_atime_unused;    //未实现
        time_t  fi_mtime_unused;
};
//内存inode结构
struct inode {
        struct finode  i_finode;
        struct inode    *i_parent;    //所属的目录i节点
        int    i_ino;            //i节点号
        int    i_users;        //引用计数
};
//目录项结构(非Linux内核的目录项)
struct direct
{
        char d_name[MAXLEN];        //文件或者目录的名字
        unsigned short d_ino;        //文件或者目录的i节点号
};
//目录结构
struct dir
{
        struct direct direct[DIRNUM];    //包含的目录项数组
        unsigned short size;        //包含的目录项大小    
};
//抽象的文件结构
struct file {
        struct inode *f_inode;        //文件的i节点
        int f_curpos;            //文件的当前读写指针
};

//内存i节点数组，NUM为该文件系统容纳的文件数
struct inode g_usedinode[NUM];
//ROOT的内存i节点
struct inode *g_root;
//已经打开文件的数组
struct file* g_opened[OPENNUM];

//超级块
struct filesys *g_super;

//模拟二级文件系统的Linux大文件的文件描述符
int g_fake_disk = -1;
int g_fd=0;


//同步i节点，将其写入“磁盘”
void syncinode(struct inode *inode)
{
        int ino = -1, ipos = -1;
        ino = inode->i_ino;
    //ipos为inode节点表在文件系统块中的偏移
        ipos = IBPOS + ino*sizeof(struct finode);
    //从模拟块的指定偏移位置读取inode信息
        lseek(g_fake_disk, ipos, SEEK_SET); //SEEK_SET 将读写位置指向文件头后再增加ipos个位移量
        write(g_fake_disk, (void *)&inode->i_finode, sizeof(struct finode));
}

//同步超级块信息
int syncsuper(struct filesys *super)
{
        int pos = -1, size = -1;
        struct dir dir = {0};
        pos = BOOTBSIZE;
        size = SUPERBSIZE;
        lseek(g_fake_disk, pos, SEEK_SET);
        write(g_fake_disk, (void *)super, size);
        syncinode(g_root);
        breadwrite(g_root->i_finode.fi_addr[0], (char *)&dir, sizeof(struct dir), 0, 1);
        breadwrite(g_root->i_finode.fi_addr[0], (char *)&dir, sizeof(struct dir), 0, 0);
}

/*
func:将路径名转换为i节点的函数，暂不支持相对路径
param:
	filepath:文件路径，第一个字符为'/'
*/
struct inode *namei(char *filepath, char flag, int *match, char *ret_name)
{
        int in = 0;
        int repeat = 0;
        char *name = NULL;
        char *path = calloc(1, MAXLEN*10);
        char *back = path;

        struct inode *root = iget(0);
        struct inode *parent = root;
        struct dir dir = {0};
        strncpy(path, filepath, MAXLEN*10);
        if (path[0] != '/')
                return NULL;
        breadwrite(root->i_finode.fi_addr[0], &dir, sizeof(struct dir), 0, 1);

		//根据字符'/'进行分割文件路径,name为每级目录
        while((name=strtok(path, "/")) != NULL) 
		{
                int i = 0;
                repeat = 0;
                *match = 0;
                path = NULL;
                if (ret_name) {
                        strcpy(ret_name, name);
                }
                for (; i<dir.size; i++) {
                        if (!strncmp(name, dir.direct[i].d_name, strlen(name))) {
                                parent = root;
                                iput(root);
                                root = iget(dir.direct[i].d_ino);
                                root->i_parent = parent;
                                *match = 1;
                                if (root->i_finode.fi_mode == MODE_DIR) {
                                        memset(&dir, 0, sizeof(struct dir));
                                        breadwrite(root->i_finode.fi_addr[0], &dir, sizeof(struct dir), 0, 1);
                                } else {
                                        free(back);
                                        return root;
                                }
                                repeat = 1;
                        }
                }
                if (repeat == 0) {
                        break;
                }
        }
        if (*match != 1) {
                *match = 0;
        }
        if (*match == 0) {
                if (ret_name) {
                        strcpy(ret_name, name);
                }
        }
        free(back);
        return root;
}

/*
func:通过i节点号获取内存i节点的函数
param:
	ino:inode 节点号
*/
struct inode *iget(unsigned int ino)
{
        int ibpos = 0;
        int ipos = 0;
        int ret = 0;
		
    //倾向于直接从内存i节点获取
        if (g_usedinode[ino].i_users) {
                g_usedinode[ino].i_users ++;
                return &g_usedinode[ino];
        }
        if (g_fake_disk < 0) {
                return NULL;
        }
    //如果内存中没有该i节点，则从模拟磁盘块读入
        ipos = IBPOS + ino*sizeof(struct finode);
        lseek(g_fake_disk, ipos, SEEK_SET);
        ret = read(g_fake_disk, &g_usedinode[ino], sizeof(struct finode));
        if (ret == -1) {
                return NULL;
        }
		
        if (g_super->s_freeinode[ino] == 0) {
                return NULL;
        }
    //如果是一个已经被删除的文件或者从未被分配过的i节点，则初始化其link值以及size值
        if (g_usedinode[ino].i_finode.fi_nlink == 0) {
                g_usedinode[ino].i_finode.fi_nlink ++;
                g_usedinode[ino].i_finode.fi_size = 0;
                syncinode(&g_usedinode[ino]);
        }
        g_usedinode[ino].i_users ++;
        g_usedinode[ino].i_ino = ino;
        return &g_usedinode[ino];

}

//释放一个占有的内存i节点
void iput(struct inode *ip)
{
        if (ip->i_users > 0)
                ip->i_users --;
}
//分配一个未使用的i节点。注意，我并没有使用超级块的s_freeinodesize字段，
//因为还会有一个更好更快的分配算法
struct inode* ialloc()
{
        int ino = -1, nowpos = -1;
        ino = g_super->s_nextfreeinode;
        if (ino == -1) {
                return NULL;
        }
        nowpos = ino + 1;
        g_super->s_nextfreeinode = -1;
    //寻找下一个空闲i节点，正如上述，这个算法并不好
        for (; nowpos < NUM; nowpos++) {
                if (g_super->s_freeinode[nowpos] == 0) {
                        g_super->s_nextfreeinode = nowpos;
                        break;
                }
        }
        g_super->s_freeinode[ino] = 1;
        return iget(ino);
}
//试图删除一个文件i节点
int itrunc(struct inode *ip)
{
        iput(ip);
        if (ip->i_users == 0 && g_super) {
                syncinode(ip);
                g_super->s_freeinode[ip->i_ino] = 0;
                g_super->s_nextfreeinode = ip->i_ino;
                return 0;
        }
        return ERR_BUSY;
}
//分配一个未使用的磁盘块
int balloc()
{
        int bno = -1, nowpos = -1;
        bno = g_super->s_nextfreeblock;
        if (bno == -1) {
                return bno;
        }
        nowpos = bno + 1;
        g_super->s_nextfreeblock = -1;
        for (; nowpos < NUM; nowpos++) {
                if (g_super->s_freeblock[nowpos] == 0) {
                        g_super->s_nextfreeblock = nowpos;
                        break;
                }
        }
        g_super->s_freeblock[bno] = 1;
        return bno;
}
/*
func:读写操作
param: 
	bno:要读/写的位置
	buf:要读/写的字符
	size:buf的大小
	offset:偏移量(为什么会有偏移量???)
	type:读/写类型
return: 读/写的字节数
*/
int breadwrite(int bno, char *buf, int size, int offset, int type)
{
		//计算开始写入的位置
		int pos = BOOTBSIZE/*自举块大小，启动块大小*/
			+SUPERBSIZE/*超级块大小*/
			+g_super->s_itsize /*inode节点表大小*/
			+ bno*BSIZE;  /*要写入的位置*/
			
        int rs = -1;
        if (offset + size > BSIZE) {
                return ERR_EXCEED;
        }
        lseek(g_fake_disk, pos + offset, SEEK_SET);
        rs = type ? read(g_fake_disk, buf, size):write(g_fake_disk, buf, size);
        return rs;
}
//IO读接口
int mfread(int fd, char *buf, int length)
{
        struct file *fs = g_opened[fd];
        struct inode *inode = fs->f_inode;
        int baddr = fs->f_curpos;
        int bondary = baddr%BSIZE;
        int max_block = (baddr+length)/BSIZE;
        int size = 0;
        int i = inode->i_finode.fi_addr[baddr/BSIZE+1];
        for (; i < max_block+1; i ++,bondary = size%BSIZE) {
                size += breadwrite(inode->i_finode.fi_addr[i], buf+size, (length-size)%BSIZE, bondary, 1);
        }
        return size;
}
//IO写接口
int mfwrite(int fd, char *buf, int length)
{
        struct file *fs = g_opened[fd];
        struct inode *inode = fs->f_inode;
        int baddr = fs->f_curpos;
        int bondary = baddr%BSIZE;
        int max_block = (baddr+length)/BSIZE;
        int curr_blocks = inode->i_finode.fi_size/BSIZE;
        int size = 0;
        int sync = 0;
        int i = inode->i_finode.fi_addr[baddr/BSIZE+1];
    //如果第一次写，先分配一个块
        if (inode->i_finode.fi_size == 0) {
        int nbno = balloc();
                if (nbno == -1) {
                        return -1;
                }
                inode->i_finode.fi_addr[0] = nbno;
                sync = 1;
        }
    //如果必须扩展，则再分配块，可以和上面的合并优化
        if (max_block > curr_blocks) {
                int j = curr_blocks + 1;
                for (; j < max_block; j++) {
            int nbno = balloc();
                    if (nbno == -1) {
                            return -1;
                    }
                        inode->i_finode.fi_addr[j] = nbno;
                }
                sync = 1;
        }
        for (; i < max_block+1; i ++,bondary = size%BSIZE) {
                size += breadwrite(inode->i_finode.fi_addr[i], buf+size, (length-size)%BSIZE, bondary, 0);
        }
        if (size) {
                inode->i_finode.fi_size += size;
                sync = 1;
        }
        if (sync) {
                syncinode(inode);
        }
        return size;
}
//IO的seek接口
int mflseek(int fd, int pos)
{
        struct file *fs = g_opened[fd];
        fs->f_curpos = pos;
        return pos;
}
//IO打开接口
int mfopen(char *path, int mode)
{
        struct inode *inode = NULL;
        struct file *file = NULL;
        int match = 0;
        inode = namei(path, 0, &match, NULL);
        if (match == 0) {
                return ERR_NOEXIST;
        }
        file = (struct file*)calloc(1, sizeof(struct file));
        file->f_inode = inode;
        file->f_curpos = 0;
        g_opened[g_fd] = file;
        g_fd++;
        return g_fd-1;
}
//IO关闭接口
void mfclose(int fd)
{
        struct inode *inode = NULL;
        struct file *file = NULL;
        file = g_opened[fd];
        inode = file->f_inode;
        iput(inode);
        free(file);
}
//IO创建接口
int mfcreat(char *path, int mode)
{
        int match = 0;
        struct dir dir;
        struct inode *new = NULL;
        char name[MAXLEN] = {0};;
		//根据路径返回文件节点
        struct inode *inode = namei(path, 0, &match, name);
        if (match == 1) {
                return ERR_EXIST;
        }
        breadwrite(inode->i_finode.fi_addr[0], 
			(char *)&dir, 
			sizeof(struct dir), 0, 1);
        strcpy(dir.direct[dir.size].d_name, name);
        new = ialloc();
        if (new == NULL) {
                return -1;
        }
        dir.direct[dir.size].d_ino = new->i_ino;
        new->i_finode.fi_mode = mode;
        if (mode == MODE_DIR) {
        //不允许延迟分配目录项
                int nbno = balloc();
                if (nbno == -1) {
                        return -1;
                }
                new->i_finode.fi_addr[0] = nbno;
        }
        new->i_parent = inode;
        syncinode(new);
        dir.size ++;
        breadwrite(inode->i_finode.fi_addr[0], (char *)&dir, sizeof(struct dir), 0, 0);
        syncinode(inode);
        iput(inode);
        syncinode(new);
        iput(new);
        return ERR_OK;
}
//IO删除接口
int mfdelete(char *path)
{
        int match = 0;
        struct dir dir;
        struct inode *del = NULL;
        struct inode *parent = NULL;
        char name[MAXLEN];
        int i = 0;
        struct inode *inode = namei(path, 0, &match, name);
        if (match == 0 || inode->i_ino == 0) {
                return ERR_NOEXIST;
        }
        match = -1;
        parent = inode->i_parent;
        breadwrite(parent->i_finode.fi_addr[0], (char *)&dir, sizeof(struct dir), 0, 1);
        for (; i < dir.size; i++) {
                if (!strncmp(name, dir.direct[i].d_name, strlen(name))) {
                        del = iget(dir.direct[i].d_ino);
                        iput(del);
                        if (itrunc(del) == 0) {
                                memset(dir.direct[i].d_name, 0, strlen(dir.direct[i].d_name));
                                match = i;
                                break;
                        } else {
                                return ERR_BUSY;
                        }
                }
        }
        for (i = match; i < dir.size - 1 && match != -1; i++) {
                strcpy(dir.direct[i].d_name, dir.direct[i+1].d_name);
        }
        dir.size--;
        breadwrite(parent->i_finode.fi_addr[0], (char *)&dir, sizeof(struct dir), 0, 0);
        return ERR_OK;
}

//序列初始化接口，从模拟块设备初始化内存结构
int initialize(char *fake_disk_path)
{
        g_fake_disk = open(fake_disk_path, O_RDWR);
        if (g_fake_disk == -1) {
                return ERR_NOEXIST;
        }
		
		//calloc在动态分配完内存后，自动初始化该内存空间为零，而malloc不初始化
        g_super = (struct filesys*)calloc(1, sizeof(struct filesys));
        lseek(g_fake_disk, BOOTBSIZE, SEEK_SET);
        read(g_fake_disk, g_super, sizeof(struct filesys));
        g_super->s_size = 1024*1024;
        g_super->s_itsize = INODEBSIZE;
        g_super->s_freeinodesize = NUM;
        g_super->s_freeblocksize = (g_super->s_size - (BOOTBSIZE+SUPERBSIZE+INODEBSIZE))/BSIZE;
        g_root =  iget(0);
    //第一次的话要分配ROOT
        if (g_root == NULL) {
                g_root = ialloc();
                g_root->i_finode.fi_addr[0] = balloc();
        }
        return ERR_OK;
}

int main()
{
        int fd = -1,ws = -1;
        char buf[16] = {0};
        initialize("bigdisk");  //大文件模拟磁盘路径
        mfcreat("/aa", MODE_FILE);
        fd = mfopen("/aa", 0);
        ws = mfwrite(fd, "abcde", 5);
        mfread(fd, buf, 5);
        mfcreat("/bb", MODE_DIR);
        mfcreat("/bb/cc", MODE_FILE);
        fd = mfopen("/bb/cc", 0);
        ws = mfwrite(fd, "ABCDEFG", 6);
        mfread(fd, buf, 5);
        mflseek(0, 4);
        ws = mfwrite(0, "ABCDEFG", 6);
        mflseek(0, 0);
        mfread(0, buf, 10);
        mfclose(0);
        mfdelete("/aa");
        fd = mfopen("/aa", 0);
        mfcreat("/aa", MODE_FILE);
        fd = mfopen("/aa", 0);
        syncsuper(g_super);
}
