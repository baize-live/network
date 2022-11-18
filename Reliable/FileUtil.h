#ifndef FILE_V2_H
#define FILE_V2_H

#if defined (WIN32) || defined (WIN64) || defined (_MSC_VER)

#include <io.h>
#include <direct.h>

#elif defined(_linux) || defined(MINGW)
#include <io.h>
#include <direct.h>
#endif

#include <string>
#include <list>
#include <vector>

/** �ú�����������·���е�"\\" �滻�� "/"��ͬʱ��·��β���"/"ȥ��
 */
void sepReplace(std::string &sPath);

/** ����һ��·�����жϸ�·�����ļ������ļ���
  * ����ֵΪ1����Ϊ�ļ�������2��Ϊ�ļ��У�����0��Ϊ���󣬿��ܸ�·�����ǳ����ļ�/�ļ���·��
  */
int isFileOrDir(const std::string &sPath);

/**
 * @brief
 * �Ӹ������ļ����õ���༶��Ŀ¼���������Ӧ˳��洢��vSubDirs��
 * �磺str = "\\DirA\\DirB\\DirC\\FileA"
 * ��ת�����vSubDirsΪ:[
 *                     DirA,
 *                     DirA/DirB,
 *                     DirA/DirB/DirC,
 *                     DirA/DirB/DirC/FileA
 *                    ]
 * @param str
 * @param vSubDirs
 */
void getMultiLayerDirFromStr(const std::string &str, std::vector<std::string> &vSubDirs);

/**
 * @brief �ַ����ָ�������ո����ķָ��ַ�cSep���зָ�ָ����洢��vTokenRes��
 * @param str
 * @param cSep
 * @param vTokenRes
 */
void strToken(const std::string &str, char cSep, std::vector<std::string> &vTokenRes);

class Dir;

class File {
public:
    explicit File(const std::string &sFilePath = "");

    virtual ~File();

    void setFilePath(const std::string &sFilePath);

    std::string fileName();

    const std::string &filePath();

    int fileSize();

    static int fileSize(const std::string &sFileName);

    bool isFile();

    bool isDir();

    // ���ص�ǰ�ļ�����Ŀ¼�Ķ���
    Dir parentDir();

    // ��ǰ�ļ����´����ļ�
    bool touch(const std::string &sFileName);

    // ָ���ļ��У������´������ļ�
    static bool touch(const std::string &sTarDirName, const std::string &sNewFileName);

    // ����ǰ�ļ����Ƶ�Ŀ���ļ�����
    virtual bool copy(const std::string &sTarDirPath);

    // ָ���ļ��У���ָ���ļ�������ָ���ļ�����
    static bool copy(const std::string &sTarDirName, const std::string &sFilePath);

protected:
    std::string m_sFilePath;
    int m_iIsFileOrDir;
};

typedef std::list<File *> FileList;

class Dir : public File {
public:
    explicit Dir(const std::string &sDirPath = "");

    ~Dir() override;

    bool mkdir(const std::string &sNewDirName);

    static bool mkdir(const std::string &sTarDirPath, const std::string &sNewDirName);


    // ������ǰ�ļ����µ������ļ�
    // ���bDeepTraverseΪFALSE����ֻ������ǰĿ¼
    // ���bDeepTraverseΪTRUE�� �������ǰĿ¼��������Ŀ¼�����ļ�
    const FileList &entry(bool bDeepTraverse = false);

    // ��̬����
    static FileList entry_static(const std::string &sDirPath, bool bDeepTraverse = false);      // ???

    // ���غ���������ǰ�ļ��и��Ƶ�Ŀ���ļ�����
    bool copy(const std::string &sTarDirPath) override;

    // ��һ���������ļ��У����Ƶ�Ŀ���ļ��У�����Ŀ���ļ����´�����ǰ�ļ��У�ֻ�ǰѵ�ǰ�ļ�������������ݸ��Ƶ�Ŀ���ļ����¡�
    static bool copy(const std::string &sSrcDirPath, const std::string &sTarDirPath);

protected:
    static void listFiles(const std::string &sDirPath, FileList &list);

    static void listFiles_Deep(const std::string &sDirPath, FileList &list);

protected:
    FileList m_lstFileList;
};

// ��ȡ�������ļ�����ڸ������ļ��е����·��
// �ļ���Ҫ�������ļ����У�����ļ����������ļ��У����ؿ��ַ���
std::string getRelativePath(File &file, Dir &dir);

#endif // FILE_V2_H

