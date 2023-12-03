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

/** 该函数将给定的路径中的"\\" 替换成 "/"，同时将路径尾后的"/"去掉
 */
void sepReplace(std::string &sPath);

/** 给定一个路径，判断该路径是文件还是文件夹
  * 返回值为1，则为文件，返回2则为文件夹，返回0则为错误，可能改路径不是常规文件/文件夹路径
  */
int isFileOrDir(const std::string &sPath);

/**
 * @brief
 * 从给定的文件，得到其多级子目录，按照其对应顺序存储到vSubDirs中
 * 如：str = "\\DirA\\DirB\\DirC\\FileA"
 * 则转换后的vSubDirs为:[
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
 * @brief 字符串分割函数，按照给定的分割字符cSep进行分割。分割结果存储在vTokenRes中
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

    // 返回当前文件所在目录的对象
    Dir parentDir();

    // 当前文件夹下创建文件
    bool touch(const std::string &sFileName);

    // 指定文件夹，在其下创建新文件
    static bool touch(const std::string &sTarDirName, const std::string &sNewFileName);

    // 将当前文件复制到目标文件夹下
    virtual bool copy(const std::string &sTarDirPath);

    // 指定文件夹，将指定文件拷贝到指定文件夹下
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


    // 遍历当前文件夹下的所有文件
    // 如果bDeepTraverse为FALSE，则只遍历当前目录
    // 如果bDeepTraverse为TRUE， 则遍历当前目录的所有子目录及其文件
    const FileList &entry(bool bDeepTraverse = false);

    // 静态函数
    static FileList entry_static(const std::string &sDirPath, bool bDeepTraverse = false);      // ???

    // 重载函数，将当前文件夹复制到目标文件夹下
    bool copy(const std::string &sTarDirPath) override;

    // 将一个给定的文件夹，复制到目标文件夹，不在目标文件夹下创建当前文件夹，只是把当前文件夹里的所有内容复制到目标文件夹下。
    static bool copy(const std::string &sSrcDirPath, const std::string &sTarDirPath);

protected:
    static void listFiles(const std::string &sDirPath, FileList &list);

    static void listFiles_Deep(const std::string &sDirPath, FileList &list);

protected:
    FileList m_lstFileList;
};

// 获取给定的文件相对于给定的文件夹的相对路径
// 文件需要存在于文件夹中，如果文件不存在与文件夹，返回空字符串
std::string getRelativePath(File &file, Dir &dir);

#endif // FILE_V2_H

