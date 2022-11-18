#include "FileUtil.h"
#include <fstream>

void sepReplace(std::string &sPath) {
    int iPos = 0;
    while (sPath.find('\\') != sPath.npos) {
        iPos = sPath.find('\\', iPos);
        sPath = sPath.replace(iPos, 1, "/");
    }

    if (sPath.back() == '/')
        sPath.pop_back();
}

int isFileOrDir(const std::string &sPath) {
    intptr_t handle;
    _finddata_t findData;

    handle = _findfirst(sPath.c_str(), &findData);
    if (findData.attrib & _A_SUBDIR)
        return 2;
    else
        return 1;

    _findclose(handle);
}

void getMultiLayerDirFromStr(const std::string &str, std::vector<std::string> &vSubDirs) {
    std::string sOriStrCp = str;
    // ��ԭʼ�ַ�����"\\"�滻��"/"
    sepReplace(sOriStrCp);
    // ���ַ������зָ�
    std::vector<std::string> vTokenRes;
    strToken(sOriStrCp, '/', vTokenRes);
    // ���ָ��Ľ�����������ۼӵķ�ʽ���洢��vSubDirs��
    vSubDirs.push_back(vTokenRes.front());
    for (int i = 1; i < (int) vTokenRes.size(); i++) {
        vSubDirs.push_back(vSubDirs[i - 1] + "/" + vTokenRes[i]);
    }
}

void strToken(const std::string &str, const char cSep, std::vector<std::string> &vTokenRes) {
    int iPos = 0;
    while (str.find(cSep, iPos) != str.npos) {
        vTokenRes.push_back(str.substr(iPos, str.find(cSep, iPos)));
        iPos = str.find(cSep, iPos) + 1;
    }
    vTokenRes.push_back(str.substr(iPos));
}

File::File(const std::string &sFilePath) :
        m_sFilePath(sFilePath) {
    m_iIsFileOrDir = isFileOrDir(m_sFilePath);
}

File::~File() {

}

void File::setFilePath(const std::string &sFilePath) {
    this->m_sFilePath = sFilePath;
    sepReplace(m_sFilePath);
}

std::string File::fileName() {
    return m_sFilePath.substr(m_sFilePath.find_last_of('/') + 1);
}

const std::string &File::filePath() {
    return m_sFilePath;
}

int File::fileSize() {
    FILE *file = fopen(m_sFilePath.c_str(), "rb");
    if (file) {
        int size = filelength(fileno(file));
        fclose(file);
        return size;
    }
    return -1;
}

int File::fileSize(const std::string &sFileName) {
    FILE *file = fopen(sFileName.c_str(), "rb");
    if (file) {
        int size = filelength(fileno(file));
        fclose(file);
        return size;
    }
    return -1;
}

bool File::isFile() {
    return m_iIsFileOrDir == 1;
}

bool File::isDir() {
    return m_iIsFileOrDir == 2;
}

Dir File::parentDir() {
    Dir pD;
    pD.setFilePath(this->m_sFilePath.substr(0, m_sFilePath.find_last_of('/')));

    return pD;
}

bool File::touch(const std::string &sFileName) {
    std::string sDirPath = this->parentDir().filePath();

    std::string sNewFilePath;
    sNewFilePath = sDirPath + std::string("/") + std::string(sFileName);

    /* _access����˵�������ο����ӣ�https://blog.csdn.net/monk1992/article/details/81906013��
     * ͷ�ļ���<io.h>
     * ����ԭ�ͣ�int _access(const char *pathname, int mode);
     * ������pathname Ϊ�ļ�·����Ŀ¼·�� mode Ϊ����Ȩ�ޣ��ڲ�ͬϵͳ�п����ò��ܵĺ궨�����¶��壩
     * ����ֵ������ļ�����ָ���ķ���Ȩ�ޣ���������0������ļ������ڻ��߲��ܷ���ָ����Ȩ�ޣ��򷵻�-1.
     * ��ע����pathnameΪ�ļ�ʱ��_access�����ж��ļ��Ƿ���ڣ����ж��ļ��Ƿ������modeֵָ����ģʽ���з��ʡ���pathnameΪĿ¼ʱ��_accessֻ�ж�ָ��Ŀ¼�Ƿ���ڣ���Windows NT��Windows 2000�У����е�Ŀ¼��ֻ�ж�дȨ�ޡ�
     * mode��ֵ�ͺ���������ʾ��
     * 00����ֻ����ļ��Ƿ����
     * 02����дȨ��
     * 04������Ȩ��
     * 06������дȨ��
    */
    if (0 != _access(sDirPath.c_str(), 2)) {
        printf("***ERROR***: Dest dir has no writing access.\n");
        return false;
    }

    std::ofstream os(sNewFilePath, std::ios::out);
    if (os.is_open()) {
        os.close();
        return true;
    } else {
        os.close();
        return false;
    }
}

bool File::touch(const std::string &sTarDirName, const std::string &sNewFileName) {
    std::string sParent = sTarDirName;
    sepReplace(sParent);

    if (0 != _access(sParent.c_str(), 2)) {
        printf("***ERROR***: Dest dir has no writing access.\n");
        return false;
    }

    std::string sNewFilePath = sParent;

    // ���sNewFileName�ǰ����༶Ŀ¼���ļ����磺DirA\\DirB\\DirB\\FileA.txt;
    // ����Ҫ�����ļ�·����ֿ����ֱ𴴽��༶Ŀ¼���ٴ����ļ�
//    if(sNewFileName.find('/') != sNewFileName.npos || sNewFileName.find('\\') != sNewFileName.npos)
//    {
//        std::vector<std::string> sSubDirs;
//        getMultiLayerDirFromStr(sNewFileName, sSubDirs);
//        for(int i = 0; i < (int)sSubDirs.size() - 1; i++)
//        {
//            Dir::mkdir(sTarDirName, sSubDirs[i]);
//        }
//    }

    sNewFilePath.append("/").append(sNewFileName);

    std::ofstream os(sNewFilePath, std::ios::out);
    if (os.is_open()) {
        os.close();
        return true;
    } else {
        os.close();
        return false;
    }
}

bool File::copy(const std::string &sTarDirPath) {
    if (0 != _access(sTarDirPath.c_str(), 2)) {
        printf("***ERROR***: Dest dir has no writing access.\n");
        return false;
    }

    /* �����ļ����Ƶ�pTarDirPath�� */
    // 1. ���ļ��������Ʒ�ʽ��
    std::ifstream inFile(m_sFilePath, std::ios::in | std::ios::binary);
    if (!inFile.is_open()) {
        printf("***ERROR***: File open failure.\n");
        inFile.close();
        return false;
    }

    // 2. ��Ŀ���ļ����´����µ�ǰͬ���ļ�
    // ���������жϸ��ļ��Ƿ����
    bool touchRes = File::touch(sTarDirPath, this->fileName().c_str());
    if (touchRes == false) {
        printf("***ERROR***: New file created to target dir failed.\n");
        inFile.close();
        return false;
    }

    std::string sNewFilePath = std::string(sTarDirPath) + "/" + this->fileName();

    // 3. �򿪴����ĸ����ļ����������ݸ��Ƶ����ļ���
    std::ofstream outFile(sNewFilePath, std::ios::out | std::ios::binary);
    if (!outFile.is_open()) {
        printf("***ERROR***: Open copied new file failed.\n");
        inFile.close();
        outFile.close();
        return false;
    }

    outFile << inFile.rdbuf();  // ��仰����Ҳ�������õģ�˵�ٶȺܿ�

    inFile.close();
    outFile.close();

    return true;
}

bool File::copy(const std::string &sTarDirName, const std::string &sFilePath) {
    if (0 != _access(sTarDirName.c_str(), 2)) {
        printf("***ERROR***: Dest dir has no writing access.\n");
        return false;
    }

    std::ifstream inFile(sFilePath, std::ios::in | std::ios::binary);
    if (!inFile.is_open()) {
        printf("***ERROR***: Open file failure.\n");
        inFile.close();
        return false;
    }

    // ��Ŀ���ļ����´������ļ�
    std::string sTarDir = sTarDirName;
    sepReplace(sTarDir);
    File oriFile(sFilePath);
    bool touchRes = File::touch(sTarDir, oriFile.fileName());
    if (!touchRes) {
        printf("***ERROR***: Touch new file failure.\n");
        inFile.close();
        return false;
    }

    std::string sNewFilePath = sTarDir + "/" + oriFile.fileName();

    // 3. �򿪴����ĸ����ļ����������ݸ��Ƶ����ļ���
    std::ofstream outFile(sNewFilePath, std::ios::out | std::ios::binary);
    if (!outFile.is_open()) {
        printf("***ERROR***: Open copied new file failed.\n");
        inFile.close();
        outFile.close();
        return false;
    }

    outFile << inFile.rdbuf();  // ��仰����Ҳ�������õģ�˵�ٶȺܿ�

    inFile.close();
    outFile.close();

    return true;
}

Dir::Dir(const std::string &sDirPath) {
    m_sFilePath = sDirPath;
    sepReplace(m_sFilePath);

    m_iIsFileOrDir = isFileOrDir(m_sFilePath);
}

Dir::~Dir() {
    while (!m_lstFileList.empty()) {
        auto file = m_lstFileList.front();
        if (file != nullptr) {
            delete file;
            m_lstFileList.pop_front();
        }
    }
}

bool Dir::mkdir(const std::string &sNewDirName) {
    if (0 != _access(m_sFilePath.c_str(), 2)) {
        printf("***ERROR***: Curent dir has no writing access.\n");
        return false;
    }

    // ���sNewDirName�Ƕ༶��Ŀ¼��ɵģ��������в�ִ���
    std::vector<std::string> vSubDirs;
    getMultiLayerDirFromStr(sNewDirName, vSubDirs);
    for (int i = 0; i < (int) vSubDirs.size(); i++) {
        std::string sSubDirPath = sNewDirName + vSubDirs[i];
        _mkdir(sSubDirPath.c_str());
    }

//    std::string sNewDirPath;
//    sNewDirPath = m_sFilePath + std::string("/") + sNewDirName;
//    int iMkdirRes = _mkdir(sNewDirPath.c_str());

    return true;
}

bool Dir::mkdir(const std::string &sTarDirPath, const std::string &sNewDirName) {
    std::string sParentDirPath = sTarDirPath;
    sepReplace(sParentDirPath);
    if (0 != _access(sParentDirPath.c_str(), 2)) {
        printf("***ERROR***: Dest dir has no writing access.\n");
        return false;
    }

    // ���sNewDirName�Ƕ༶��Ŀ¼��ɵģ��������в�ִ���
    std::vector<std::string> vSubDirs;
    getMultiLayerDirFromStr(sNewDirName, vSubDirs);
    for (int i = 0; i < (int) vSubDirs.size(); i++) {
        std::string sSubDirPath = sTarDirPath + "/" + vSubDirs[i];
        _mkdir(sSubDirPath.c_str());
    }

//    std::string sNewDirPath;
//    sNewDirPath = sParentDirPath + std::string("/") + sNewDirName;
//    int iMkdirRes = _mkdir(sNewDirPath.c_str());

    return true;
}

const FileList &Dir::entry(bool bDeepTraverse) {
    if (bDeepTraverse == false)
        listFiles(m_sFilePath, m_lstFileList);
    else
        listFiles_Deep(m_sFilePath, m_lstFileList);
    return m_lstFileList;
}

FileList Dir::entry_static(const std::string &sDirPath, bool bDeepTraverse) {
    FileList list;
    if (bDeepTraverse == false)
        listFiles(sDirPath, list);
    else
        listFiles_Deep(sDirPath, list);
    return list;
}

bool Dir::copy(const std::string &sTarDirPath) {
    // �ж�Ŀ���ļ����Ƿ���дȨ��
    if (0 != _access(sTarDirPath.c_str(), 2)) {
        printf("***ERROR***: Dest dir path has no writing access.\n");
        return false;
    }
    auto curFiles = this->entry(true);
    for (auto file: curFiles) {
        if (file->isFile()) {
            // ����ǰ�ļ����ļ�·����ȥ��ǰ�ļ��е�·�����õ���ǰ�ļ���Ե�ǰ�ļ��е����·��
            std::string sRelPath = getRelativePath(*file, *this);
            // ���·����ǰ׺
            std::string sPrefix;
            if (sRelPath.find('/') != sRelPath.npos)
                sPrefix = sRelPath.substr(0, sRelPath.find_last_of('/'));
            else
                sPrefix = "";
            // ��Ŀ��Ŀ¼�´������·����ǰ׺
            if (sPrefix == "") {
                File::copy(sTarDirPath, file->filePath());
            } else {
                Dir::mkdir(sTarDirPath, sPrefix);
                File::copy(sTarDirPath + "/" + sPrefix, file->filePath()); // ??????????????????????????  TODO
            }
        }
    }

    // �ڴ���� ���� curFiles����entry_static�д����ģ������Ҫ�ֶ�����
    while (!curFiles.empty()) {
        auto file = curFiles.front();
        if (file != nullptr) {
            delete file;
            curFiles.pop_front();
        }
    }
    return true;
}

bool Dir::copy(const std::string &sSrcDirPath, const std::string &sTarDirPath) {
    // �ж϶�Ŀ���ļ����Ƿ���дȨ��
    if (0 != _access(sTarDirPath.c_str(), 2)) {
        printf("***ERROR***: Dest dir path has no writing access.\n");
        return false;
    }

    Dir srcDir(sSrcDirPath);

    // ������ǰ�ļ��У��õ����е��ļ�
    auto curFiles = Dir::entry_static(sSrcDirPath, true);
    for (auto file: curFiles) {
        if (file->isFile()) {
            // ����ǰ�ļ����ļ�·����ȥ��ǰ�ļ��е�·�����õ���ǰ�ļ���Ե�ǰ�ļ��е����·��
            std::string sRelPath = getRelativePath(*file, srcDir);
            // ���·����ǰ׺
            std::string sPrefix;
            if (sRelPath.find('/') != sRelPath.npos)
                sPrefix = sRelPath.substr(0, sRelPath.find_last_of('/'));
            else
                sPrefix = "";
            // ��Ŀ��Ŀ¼�´������·����ǰ׺
            if (sPrefix == "") {
                File::copy(sTarDirPath, file->filePath());
            } else {
                Dir::mkdir(sTarDirPath, sPrefix);
                File::copy(sTarDirPath + "/" + sPrefix, file->filePath()); // ??????????????????????????  TODO
            }
        }
    }

    // �ڴ���� ���� curFiles����entry_static�д����ģ������Ҫ�ֶ�����
    while (!curFiles.empty()) {
        auto file = curFiles.front();
        if (file != nullptr) {
            delete file;
            curFiles.pop_front();
        }
    }
    return true;
}

void Dir::listFiles(const std::string &sDirPath, FileList &list) {
    intptr_t handle;
    _finddata_t findData;

    std::string sDir = sDirPath;
    sepReplace(sDir);

    std::string sAddWildcardCharacter = sDir + "/*.*";   // �����������ͨ���������ֻѭ��һ��
//    std::string sAddWildcardCharacter = m_sFilePath + "/";    // ����Ŀ¼���б�ܵĲ��У���findʧ��

    handle = _findfirst(sAddWildcardCharacter.c_str(), &findData);
    if (-1 == handle) {
        printf("***ERROR***: Find first file failure.\n");
        return;
    }

    do {
        if (findData.attrib & _A_SUBDIR) {
            if (strcmp(findData.name, ".") == 0 || strcmp(findData.name, "..") == 0)
                continue;

            Dir *pDir = new Dir(sDir + "/" + findData.name);
            list.push_back(pDir);
        } else {
            File *pFile = new File(sDir + "/" + findData.name);
            list.push_back(pFile);
        }
    } while (_findnext(handle, &findData) == 0);

    _findclose(handle);
}

void Dir::listFiles_Deep(const std::string &sDirPath, FileList &list) {
    intptr_t handle;
    _finddata_t findData;

    std::string sDir = sDirPath;
    sepReplace(sDir);

    std::string sAddWildcardCharacter = sDir + "/*.*";   // �����������ͨ���������ֻѭ��һ��
//    std::string sAddWildcardCharacter = m_sFilePath + "/";    // ����Ŀ¼���б�ܵĲ��У���findʧ��

    handle = _findfirst(sAddWildcardCharacter.c_str(), &findData);
    if (-1 == handle) {
        printf("***ERROR***: Find first file failure.\n");
        return;
    }

    do {
        if (findData.attrib & _A_SUBDIR) {
            if (strcmp(findData.name, ".") == 0 || strcmp(findData.name, "..") == 0)
                continue;

            Dir *pDir = new Dir(sDir + "/" + findData.name);
            list.push_back(pDir);

            if (pDir->fileName().back() == '.')
                continue;
            listFiles_Deep(sDir + "/" + findData.name, list);
        } else {
            File *pFile = new File(sDir + "/" + findData.name);
            list.push_back(pFile);
        }
    } while (_findnext(handle, &findData) == 0);

    _findclose(handle);
}

std::string getRelativePath(File &file, Dir &dir) {
    std::string sFilePath = file.filePath();
    std::string sDirPath = dir.filePath();

    if (sFilePath.find(sDirPath) == std::string::npos)
        return "";

    return sFilePath.substr(sDirPath.length() + 1);
}

