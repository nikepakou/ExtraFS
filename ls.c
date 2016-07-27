#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/ioctl.h>
#include<dirent.h>
#include<pwd.h>
#include<grp.h>
#include<unistd.h>

/*head file*/


#define LenOfName 256
#define maxN 1005
#define maxM 505
#define maxL 105
#define LenOfPath 256<<4
#define LS 0      //ls
#define LS_A 1    //ls -a
#define LS_L 2    //ls -l
#define LS_TMP 3  //ls /tmp
#define LS_T 5    //ls -t
/*define file*/


void do_ls( int,char [] );
void dostat( char * );                                      /*get file info*/
void show_file_info( char *,struct stat * );  //ls -l �����
void mode_to_letters( int,char [] );            //�ļ����ԣ�drwxr-xr-x
char *uid_to_name( uid_t );              /* ͨ��uid��ö�Ӧ���û��� */
char *gid_to_name( gid_t );              /* ͨ��gid��ö�Ӧ������ */
void getcolor( char * );                    
int get_file_type( char * );            /*get file type*/
int get_modify_time( char * );        /*get file last modify time*/
void getWidth();
int cmp1( const void * ,const void * );  //�ļ������� 
int cmp2( const void * ,const void * );  //�ļ��޸�ʱ������

 
struct outputFile{
    char FileName[ LenOfName ];
    int modify_time ; 
    int file_type ;
}Output[ maxN ],OutputPoint[ maxM ],Temp[ maxN+maxM ];
int colormode,foreground,background;
int terminalWidth ;

void dostat( char *filename ){
    struct stat info;
    if( stat( filename,&info )==-1 ){
        perror( filename );
        printf("filename:%s\n", filename);
    }
    else{
        char *pname = strrchr(filename, '/');
        getcolor(filename );
        show_file_info( pname+1,&info );
    }
    return ;
}
/*get file info*/

void mode_to_letters( int mode,char str[] ){
    strcpy( str,"----------" );
    
    if( S_ISDIR( mode ) ) str[0] = 'd';
    if( S_ISCHR( mode ) ) str[0] = 'c';
    if( S_ISBLK( mode ) ) str[0] = 'b';
    
    if( mode&S_IRUSR ) str[1] = 'r';
    if( mode&S_IWUSR ) str[2] = 'w';
    if( mode&S_IXUSR ) str[3] = 'x';
    
    if( mode&S_IRGRP ) str[4] = 'r';
    if( mode&S_IWGRP ) str[5] = 'w';
    if( mode&S_IXGRP ) str[6] = 'x';
    
    if( mode&S_IROTH ) str[7] = 'r';
    if( mode&S_IWOTH ) str[8] = 'w';
    if( mode&S_IXOTH ) str[9] = 'x';
    
    return ;
}


char *uid_to_name( uid_t uid ){
    struct passwd  *pw_ptr;
    static char numstr[ 10 ];
    if( (pw_ptr = getpwuid( uid ) )==NULL ){
        sprintf(numstr,"%d",uid);
        return numstr;
    }
    else{
        return pw_ptr->pw_name;
    }
}

/* ͨ��uid��ö�Ӧ���û��� */

char *gid_to_name( gid_t gid ){
    struct group  *grp_ptr;
    static char numstr[ 10 ];
    if( (grp_ptr = getgrgid( gid ) )==NULL ){
        sprintf(numstr,"%d",gid);
        return numstr;
    }
    else{
        return grp_ptr->gr_name;
    }
}
/* ͨ��gid��ö�Ӧ������ */


int get_file_type( char *filename ){
    struct stat info;
    stat( filename,&info );
    int file_type = 0;
    file_type = info.st_mode & S_IFMT;
    return file_type;
}
/*get file type*/


int get_modify_time( char *filename ){
    struct stat info;
    stat( filename,&info );
    int modify_time = 0;
    modify_time = info.st_mtime;
    return modify_time;
}
/*get file last modify time*/


int isadir(char *str)
{
    struct stat info;
    return ( lstat(str,&info) != -1 && S_ISDIR(info.st_mode) );
}
/*�ж�ĳ�ļ��Ƿ�ΪĿ¼�ļ�*/

 

void getWidth(){
    char *tp;
    struct winsize wbuf;
    terminalWidth = 80;
    if( isatty(STDOUT_FILENO) ){  
        if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &wbuf) == -1 || wbuf.ws_col == 0){
            if( tp = getenv("COLUMNS") )
                terminalWidth = atoi( tp );
        }
        else
            terminalWidth = wbuf.ws_col;
    }
    return ;
}
/******************************************************************
    �ó��ն˵Ŀ�ȣ�Ĭ�Ͽ��Ϊ80�������ȡʧ�ܣ�������-1ѡ��
*******************************************************************/

 

int cmp1( const void *p ,const void *q ){
    char T1[ LenOfName ],T2[ LenOfName ];
    strcpy( T1,(*(struct outputFile *)p).FileName );
    strcpy( T2,(*(struct outputFile *)q).FileName );
    int len1 = strlen( T1 );
    int i ;
    for( i=0;i<len1;i++ ){
        if( T1[ i ]>='A' && T1[ i ]<='Z' ){
            T1[ i ] = T1[ i ] - 'A' + 'a';
        }
    }
    int len2 = strlen( T2 );
    for( i=0;i<len2;i++ ){
        if( T2[ i ]>='A' && T2[ i ]<='Z' ){
            T2[ i ] = T2[ i ] - 'A' + 'a';
        }
    }
    return strcmp( T1,T2 );
}
/********************************************************
    �ļ�������  cmp1
*********************************************************/

int cmp2( const void *p,const void *q ){
    return (*(struct outputFile *)p).modify_time < (*(struct outputFile *)q).modify_time ;
}
/********************************************************
    �ļ��޸�ʱ������  cmp2
*********************************************************/

void show_file_info( char *filename,struct stat * info_p ){
    char modestr[ 12 ];
    
    mode_to_letters( info_p->st_mode,modestr );
    
    printf("%s",modestr);
    printf("%3d ",(int)info_p->st_nlink);
    printf("%-2s ",uid_to_name(info_p->st_uid));
    printf("%-1s ",gid_to_name(info_p->st_gid));
    printf("%7ld ",(long)info_p->st_size);
    printf("%.12s ",4+ctime(&info_p->st_mtime));
    printf("\033[%d;%d;%dm%s\033[0m\n",colormode,foreground,background,filename);
    return ;
}
/*******************************************************
            ls -l �����
********************************************************/

 

void getcolor( char *filename ){
    struct stat info;
    stat( filename,&info );
    foreground = 30;
    background = 1;
    colormode = 0;
    switch ( (info.st_mode & S_IFMT) ){
        case S_IFREG:              /*regular ��ͨ�ļ� , ɫ*/
            foreground = 30;
            colormode = 1;
            break;
        case S_IFLNK:              /*symbolic link �����ļ� , ����ɫ*/
            foreground = 36;
            colormode = 1;
            break;
        case S_IFSOCK:              /*�Ϻ�ɫ*/  
            foreground = 35;
            colormode = 1;
            break;
        case S_IFDIR:              /*directory Ŀ¼�ļ� , ��ɫ*/
            foreground = 34;
            break;
        case S_IFBLK:              /*block special ���豸�ļ� , ��ɫ*/
            foreground = 33;
            colormode = 1;
            break;
        case S_IFCHR:              /*character special �ַ��豸�ļ� , ��ɫ*/
            foreground = 33;
            colormode = 1;
            break;
        case S_IFIFO:              /*fifo  ��ɫ*/
            foreground = 32;
            colormode = 1;
            break;
    }
}
/*******************************************************
            ���ļ������ɫ
********************************************************/ 


  
void display_Ls( int cnt ){
    int wordLenMax = 0;//the LenMax word
    int wordRowNum = 0;//the amount of one row
    int wordColNum = 0;//the amount of one col
    int i , j;
    for( i=0;i<cnt;i++ ){
        if( i==0 ) wordLenMax = strlen( Output[ i ].FileName );
        else wordLenMax = wordLenMax>strlen( Output[ i ].FileName )?wordLenMax:strlen( Output[ i ].FileName );
    }
    wordLenMax +=2;
    wordRowNum = terminalWidth / wordLenMax;
    if( cnt%wordRowNum==0 ) wordColNum = cnt / wordRowNum;
    else wordColNum = cnt / wordRowNum + 1;
    for( i=0;i<wordColNum;i++ ){
        j = i;
        while( j<cnt ){
            getcolor( Output[ j ].FileName );
            printf("\033[%d;%d;%dm%-15s\033[0m ",colormode,foreground,background,Output[ j ].FileName);
            j += wordColNum;
        }
        printf("\n");
    }
    return ;
}    
/**********************************************************
            ls�ķ������
***********************************************************/

 

void display_Ls_a( int cntPoint,int cnt ){
    int CNT = 0;
    int wordLenMax = 0;//the LenMax word
    int wordRowNum = 0;//the amount of one row
    int wordColNum = 0;//the amount of one col
    int i , j;
    for( i=0;i<cntPoint;i++ ){
        strcpy( Temp[ CNT ].FileName,OutputPoint[ i ].FileName );
        Temp[ CNT ].file_type = OutputPoint[ i ].file_type;
        Temp[ CNT ].modify_time = OutputPoint[ i ].modify_time;
        CNT ++;
        wordLenMax = wordLenMax>strlen( OutputPoint[ i ].FileName )?wordLenMax:strlen( OutputPoint[ i ].FileName );
    }
    for( i=0;i<cnt;i++ ){
        strcpy( Temp[ CNT ].FileName,Output[ i ].FileName );
        Temp[ CNT ].file_type = Output[ i ].file_type;
        Temp[ CNT ].modify_time = Output[ i ].modify_time;
        CNT ++;
        wordLenMax = wordLenMax>strlen( Output[ i ].FileName )?wordLenMax:strlen( Output[ i ].FileName );
    }
    wordLenMax += 2;
    wordRowNum = terminalWidth / wordLenMax;
    if( CNT%wordRowNum==0 ) wordColNum = CNT / wordRowNum;
    else wordColNum = CNT / wordRowNum + 1;
    for( i=0;i<wordColNum;i++ ){
        j = i;
        while( j<CNT ){
            getcolor( Temp[ j ].FileName );
            printf("\033[%d;%d;%dm%-15s\033[0m ",colormode,foreground,background,Temp[ j ].FileName);
            j += wordColNum;
        }
        printf("\n");
    }
    return ;
}
/**********************************************************
            ls -a �ķ������
***********************************************************/

 

void display_Ls_tmp( int cnt ){
    display_Ls( cnt );
    return ;
}
/**********************************************************
            ls /tmp �ķ������
***********************************************************/


    
void do_ls( int myJudge,char myOrder[] ){

    char dirname[ maxL ];
    if( myJudge!=LS_TMP  && myJudge!=LS_L){
        strcpy( dirname,"." );
    }
    else {
        strcpy( dirname,myOrder );
    }
    DIR * dir_ptr;
    struct dirent *direntp;
    int cntOutput = 0;
    int cntOutputPoint = 0;
    char full_path[256];
    if( ( dir_ptr = opendir( dirname ) )==NULL ){
        fprintf( stderr,"my god, i cannot open %s\n",dirname );
    }
    else{
        if(myJudge!=LS_L ) {
                while( (direntp = readdir( dir_ptr ) )!=NULL ){
            if( direntp->d_name[ 0 ]=='.' ) {
                strcpy( OutputPoint[ cntOutputPoint ].FileName,direntp->d_name );
                OutputPoint[ cntOutputPoint ].file_type = get_file_type( OutputPoint[ cntOutputPoint ].FileName );
                OutputPoint[ cntOutputPoint ].modify_time = get_modify_time( OutputPoint[ cntOutputPoint ].FileName );
                cntOutputPoint ++;
            }
            else {
                strcpy( Output[ cntOutput ].FileName,direntp->d_name );
                Output[ cntOutput ].file_type = get_file_type( Output[ cntOutput ].FileName );
                Output[ cntOutput ].modify_time = get_modify_time( Output[ cntOutput ].FileName );
                cntOutput ++;
            }
        }
    }  
        if( myJudge==LS_T ){
            qsort( OutputPoint,cntOutputPoint,sizeof( OutputPoint[0] ),cmp2 );
            qsort( Output,cntOutput,sizeof( Output[0] ),cmp2 );
        }
        else {
            qsort( OutputPoint,cntOutputPoint,sizeof( OutputPoint[0] ),cmp1 );
            qsort( Output,cntOutput,sizeof( Output[0] ),cmp1 );
        }
        /**********************************************************
            Ԥ������� 
        ***********************************************************/
        if( myJudge==LS||myJudge==LS_T ){
            display_Ls( cntOutput );
            closedir( dir_ptr );
        }
        /**********************************************************
            ls  && ls -t
        ***********************************************************/
        else if( myJudge==LS_A ){
            display_Ls_a( cntOutputPoint,cntOutput );
            closedir( dir_ptr );
        }
        /**********************************************************
            ls -a  
        ***********************************************************/
        else if( myJudge==LS_L ){
                    while( (direntp = readdir( dir_ptr ) )!=NULL ){
            if( direntp->d_name[ 0 ]=='.' ) {
                strcpy( OutputPoint[ cntOutputPoint ].FileName,direntp->d_name );
                OutputPoint[ cntOutputPoint ].file_type = get_file_type( OutputPoint[ cntOutputPoint ].FileName );
                OutputPoint[ cntOutputPoint ].modify_time = get_modify_time( OutputPoint[ cntOutputPoint ].FileName );
                cntOutputPoint ++;
            }
            else {
                strcpy(full_path, dirname);
                int dir_len = strlen(dirname);
                strcpy(full_path + dir_len, direntp->d_name);
                strcpy( Output[ cntOutput ].FileName,full_path );
                Output[ cntOutput ].file_type = get_file_type( Output[ cntOutput ].FileName );
                Output[ cntOutput ].modify_time = get_modify_time( Output[ cntOutput ].FileName );

                cntOutput ++;
            }
        }
            int i;
            for( i=0;i<cntOutput;i++ )
                dostat( Output[ i ].FileName );
            closedir( dir_ptr );
        }
        /**********************************************************
            ls -l  
        ***********************************************************/
        else {
            display_Ls_tmp( cntOutput );
            closedir( dir_ptr );
        }
        /**********************************************************
            ls /tmp 
        ***********************************************************/
    }
    return ;
}/*sovle*/

int main( int argc, char *argv[] ){
    getWidth( );
    int i ;
    if( argc==1 ){
        do_ls( LS,"ls" );
    }
    else{
        int ord;
        while( (ord = getopt(argc,argv,":lat"))!=-1 ){
            switch( ord ){
                case 'a':
                    do_ls( LS_A,"ls-a" );
                    break;
                case 'l':{
                        if(argc == 2)  do_ls( LS_L,"/" );
                        else  {
                        for( i= 2;i<argc;i++ ){
                        if( argv[ i ][ 0 ]=='-' ) continue;
                      printf("%s:\n",argv[ i ]);
                      do_ls( LS_L,argv[ i ] );
                      }
                  } 
                  return;
                }
                    break;
                case 't':
                    do_ls( LS_T,"ls-t" );
                    break;
                default :
                    break;
            }
        }
        for( i=1;i<argc;i++ ){
            if( argv[ i ][ 0 ]=='-' ) continue;
            printf("%s:\n",argv[ i ]);
            do_ls( LS_TMP,argv[ i ] );
        }
    }
    return 0;
}


