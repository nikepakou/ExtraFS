#include<unistd.h>
//#include<sys/type.h>
#include <fcntl.h>
#include<time.h>
#include<stdlib.h>

#define BSIZE 20
#define NUM 32  //���� i�ڵ��С,�����ļ�ϵͳ���ɵ��ļ���
#define INODEBSIZE 64 //inode���С

//#define MODE_FILE 1 //mfcreate ������ʽ
#define MODE_DIR 1
#define MODE_FILE 2

//������ṹ
struct filesys {
        unsigned int s_size;        //�ܴ�С                                          
        unsigned int s_itsize;        //inode���С                                        
        unsigned int s_freeinodesize;    //����i�ڵ������                          
        unsigned int s_nextfreeinode;    //��һ������i�ڵ�
        unsigned int s_freeinode[NUM];    //����i�ڵ�����  0:��ʾ�����У�1:��ʾ����
        unsigned int s_freeblocksize;    //���п������          
        unsigned int s_nextfreeblock;    //��һ�����п�
        unsigned int s_freeblock[NUM];    //���п�����
        unsigned int s_pad[];        //��䵽512�ֽ�  
};
//����inode�ṹ
struct finode {
        int fi_mode;            //���ͣ��ļ�/Ŀ¼
        int fi_uid_unused;        //uid�����ںͽ����޹�����������ģ��һ��FS��δʹ�ã���ͬ
        int fi_gid_unused;
        int fi_nlink;            //����������������Ϊ0����ζ�ű�ɾ��
        long int fi_size;        //�ļ���С
        long int fi_addr[BNUM];        //�ļ���һ��ָ�룬��δʵ�ֶ༶ָ��
        time_t  fi_atime_unused;    //δʵ��
        time_t  fi_mtime_unused;
};
//�ڴ�inode�ṹ
struct inode {
        struct finode  i_finode;
        struct inode    *i_parent;    //������Ŀ¼i�ڵ�
        int    i_ino;            //i�ڵ��
        int    i_users;        //���ü���
};
//Ŀ¼��ṹ(��Linux�ں˵�Ŀ¼��)
struct direct
{
        char d_name[MAXLEN];        //�ļ�����Ŀ¼������
        unsigned short d_ino;        //�ļ�����Ŀ¼��i�ڵ��
};
//Ŀ¼�ṹ
struct dir
{
        struct direct direct[DIRNUM];    //������Ŀ¼������
        unsigned short size;        //������Ŀ¼���С    
};
//������ļ��ṹ
struct file {
        struct inode *f_inode;        //�ļ���i�ڵ�
        int f_curpos;            //�ļ��ĵ�ǰ��дָ��
};

//�ڴ�i�ڵ����飬NUMΪ���ļ�ϵͳ���ɵ��ļ���
struct inode g_usedinode[NUM];
//ROOT���ڴ�i�ڵ�
struct inode *g_root;
//�Ѿ����ļ�������
struct file* g_opened[OPENNUM];

//������
struct filesys *g_super;

//ģ������ļ�ϵͳ��Linux���ļ����ļ�������
int g_fake_disk = -1;
int g_fd=0;


//ͬ��i�ڵ㣬����д�롰���̡�
void syncinode(struct inode *inode)
{
        int ino = -1, ipos = -1;
        ino = inode->i_ino;
    //iposΪinode�ڵ�����ļ�ϵͳ���е�ƫ��
        ipos = IBPOS + ino*sizeof(struct finode);
    //��ģ����ָ��ƫ��λ�ö�ȡinode��Ϣ
        lseek(g_fake_disk, ipos, SEEK_SET); //SEEK_SET ����дλ��ָ���ļ�ͷ��������ipos��λ����
        write(g_fake_disk, (void *)&inode->i_finode, sizeof(struct finode));
}

//ͬ����������Ϣ
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
func:��·����ת��Ϊi�ڵ�ĺ������ݲ�֧�����·��
param:
	filepath:�ļ�·������һ���ַ�Ϊ'/'
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

		//�����ַ�'/'���зָ��ļ�·��,nameΪÿ��Ŀ¼
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
func:ͨ��i�ڵ�Ż�ȡ�ڴ�i�ڵ�ĺ���
param:
	ino:inode �ڵ��
*/
struct inode *iget(unsigned int ino)
{
        int ibpos = 0;
        int ipos = 0;
        int ret = 0;
		
    //������ֱ�Ӵ��ڴ�i�ڵ��ȡ
        if (g_usedinode[ino].i_users) {
                g_usedinode[ino].i_users ++;
                return &g_usedinode[ino];
        }
        if (g_fake_disk < 0) {
                return NULL;
        }
    //����ڴ���û�и�i�ڵ㣬���ģ����̿����
        ipos = IBPOS + ino*sizeof(struct finode);
        lseek(g_fake_disk, ipos, SEEK_SET);
        ret = read(g_fake_disk, &g_usedinode[ino], sizeof(struct finode));
        if (ret == -1) {
                return NULL;
        }
		
        if (g_super->s_freeinode[ino] == 0) {
                return NULL;
        }
    //�����һ���Ѿ���ɾ�����ļ����ߴ�δ���������i�ڵ㣬���ʼ����linkֵ�Լ�sizeֵ
        if (g_usedinode[ino].i_finode.fi_nlink == 0) {
                g_usedinode[ino].i_finode.fi_nlink ++;
                g_usedinode[ino].i_finode.fi_size = 0;
                syncinode(&g_usedinode[ino]);
        }
        g_usedinode[ino].i_users ++;
        g_usedinode[ino].i_ino = ino;
        return &g_usedinode[ino];

}

//�ͷ�һ��ռ�е��ڴ�i�ڵ�
void iput(struct inode *ip)
{
        if (ip->i_users > 0)
                ip->i_users --;
}
//����һ��δʹ�õ�i�ڵ㡣ע�⣬�Ҳ�û��ʹ�ó������s_freeinodesize�ֶΣ�
//��Ϊ������һ�����ø���ķ����㷨
struct inode* ialloc()
{
        int ino = -1, nowpos = -1;
        ino = g_super->s_nextfreeinode;
        if (ino == -1) {
                return NULL;
        }
        nowpos = ino + 1;
        g_super->s_nextfreeinode = -1;
    //Ѱ����һ������i�ڵ㣬��������������㷨������
        for (; nowpos < NUM; nowpos++) {
                if (g_super->s_freeinode[nowpos] == 0) {
                        g_super->s_nextfreeinode = nowpos;
                        break;
                }
        }
        g_super->s_freeinode[ino] = 1;
        return iget(ino);
}
//��ͼɾ��һ���ļ�i�ڵ�
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
//����һ��δʹ�õĴ��̿�
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
func:��д����
param: 
	bno:Ҫ��/д��λ��
	buf:Ҫ��/д���ַ�
	size:buf�Ĵ�С
	offset:ƫ����(Ϊʲô����ƫ����???)
	type:��/д����
return: ��/д���ֽ���
*/
int breadwrite(int bno, char *buf, int size, int offset, int type)
{
		//���㿪ʼд���λ��
		int pos = BOOTBSIZE/*�Ծٿ��С���������С*/
			+SUPERBSIZE/*�������С*/
			+g_super->s_itsize /*inode�ڵ���С*/
			+ bno*BSIZE;  /*Ҫд���λ��*/
			
        int rs = -1;
        if (offset + size > BSIZE) {
                return ERR_EXCEED;
        }
        lseek(g_fake_disk, pos + offset, SEEK_SET);
        rs = type ? read(g_fake_disk, buf, size):write(g_fake_disk, buf, size);
        return rs;
}
//IO���ӿ�
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
//IOд�ӿ�
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
    //�����һ��д���ȷ���һ����
        if (inode->i_finode.fi_size == 0) {
        int nbno = balloc();
                if (nbno == -1) {
                        return -1;
                }
                inode->i_finode.fi_addr[0] = nbno;
                sync = 1;
        }
    //���������չ�����ٷ���飬���Ժ�����ĺϲ��Ż�
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
//IO��seek�ӿ�
int mflseek(int fd, int pos)
{
        struct file *fs = g_opened[fd];
        fs->f_curpos = pos;
        return pos;
}
//IO�򿪽ӿ�
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
//IO�رսӿ�
void mfclose(int fd)
{
        struct inode *inode = NULL;
        struct file *file = NULL;
        file = g_opened[fd];
        inode = file->f_inode;
        iput(inode);
        free(file);
}
//IO�����ӿ�
int mfcreat(char *path, int mode)
{
        int match = 0;
        struct dir dir;
        struct inode *new = NULL;
        char name[MAXLEN] = {0};;
		//����·�������ļ��ڵ�
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
        //�������ӳٷ���Ŀ¼��
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
//IOɾ���ӿ�
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

//���г�ʼ���ӿڣ���ģ����豸��ʼ���ڴ�ṹ
int initialize(char *fake_disk_path)
{
        g_fake_disk = open(fake_disk_path, O_RDWR);
        if (g_fake_disk == -1) {
                return ERR_NOEXIST;
        }
		
		//calloc�ڶ�̬�������ڴ���Զ���ʼ�����ڴ�ռ�Ϊ�㣬��malloc����ʼ��
        g_super = (struct filesys*)calloc(1, sizeof(struct filesys));
        lseek(g_fake_disk, BOOTBSIZE, SEEK_SET);
        read(g_fake_disk, g_super, sizeof(struct filesys));
        g_super->s_size = 1024*1024;
        g_super->s_itsize = INODEBSIZE;
        g_super->s_freeinodesize = NUM;
        g_super->s_freeblocksize = (g_super->s_size - (BOOTBSIZE+SUPERBSIZE+INODEBSIZE))/BSIZE;
        g_root =  iget(0);
    //��һ�εĻ�Ҫ����ROOT
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
        initialize("bigdisk");  //���ļ�ģ�����·��
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
