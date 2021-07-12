#include <bits/stdc++.h>
#include<fstream> 
#include<utility>
#include<sys/stat.h>
#include<experimental/filesystem>
#include<filesystem>//           used the flag -lstdc++fs to use the above package instead
#include<chrono>
#include<ctime>
#include<unistd.h>
#include<openssl/sha.h>
#define pb push_back
using namespace std;
namespace fs=std::experimental::filesystem;
namespace fsnew=std::experimental::filesystem;
string root; // root -> /workspace/container_1


void init(string path=root){
	struct stat buffer;
	if(stat((path+"/.imperium").c_str(),&buffer)==0){// if .imperium folder exists(or is already initialized)
		cout<<"Repository has already been initialized\n"; 
        return;
	}
	
	string ignorePath=path+"/.imperiumignore";
	ofstream ignore(ignorePath.c_str());
	// in .imperiumignore last character / -> its a folder
	ignore<<".gitignore\n.imperium/\n.git\n.imperiumignore\n.node_modules\nscript/\nsrc/\nMakefile\n";
	// these files cannot be accessed by the user as it will crash the VCS
	ignore.close();
	path+="/.imperium";
	if(mkdir(path.c_str(),0777)) {
		cout<<"ERROR\n"; 
		return;
	}
	
	string commitLog=path+"/commit.log";
	ofstream commit(commitLog.c_str());
	commit.close();
	
	string addLog=path+"/add.log";
	ofstream add(addLog.c_str());
	add.close();

	string conflictLog=path+"/conflict.log";// for Merge conflict 
	ofstream conflict(conflictLog.c_str());
	conflict<<"False\n";
	conflict.close();
	
	bool master = (root=="/workspace/container_1");
	string branchName = root.substr(root.find_last_of("/")+1);
	if(master){
		string branchLog=path+"/branch.log";
		ofstream branch(branchLog.c_str());
		branch.close();
	}
	cout<<"Initialized an empty repository\n";
}


void addtoCache(string addPath,char type){// add/update the file/folder that is to be staged
	struct stat buffer;
	if(stat((root+"/.imperium/.add").c_str(),&buffer)) {//if .add folder does not exists create it
		if(mkdir((root+"/.imperium/.add").c_str(),0777)) {cout<<"Error could not create cache folder\n"; return;}
	}
	if(type=='d'){// if addPath is a directory
		string filename=addPath.substr(root.length());// extract the relative path of addPath (relative to root)
		string filerel=root+"/.imperium/.add"+filename;// get the location of addPath in the .add folder
		struct stat buffer3;
		if(stat(filerel.c_str(),&buffer3)!=0) // if does not exist then create one
			fsnew::create_directories(filerel);
		// do fsnew::create_directories(filerel) also copy & overwrite the content of the folder?
	}
	else if(type=='f'){// if addPath is a file
		string filename=addPath.substr(root.length());
		string filerel=root+"/.imperium/.add" + filename.substr(0,filename.find_last_of("/"));
		struct stat buffer2;
		if(stat(filerel.c_str(),&buffer2)!=0) 
			fsnew::create_directories(filerel);
		fsnew::copy_file(addPath,root+"/.imperium/.add"+filename,fsnew::copy_options::overwrite_existing);
		// copy and overwrite content from file/folder(addPath) to .add folder
	}
}


bool ignoreFolder(string addPath,vector<string> &ignoreDir)
{
	for(auto dir:ignoreDir){
		if(addPath.find(dir)!=string::npos) return true;
	}
	return false;
}


// function to check if addPath is to be ignored ,also it (updates the content of file/folder if alredy in add.log)
//	                                 |                                             |
//                  if present  in .imperiumignore or add.log              addtoCache function.
bool toBeIgnored(string addPath,int onlyImperiumIgnore=0)
{
	string file_or_dir;// storing content of .imperiumignore
	vector<string> ignorefilenames;// files to be ignored(that is found in .imperiumignore)
	vector<string> ignoreDir;// directory to be ignored(found in .imperiumignore)
	vector<pair<string,char>> addedFileNames;// storing content of add.log as pair -> {path,type}
	ifstream addFIle, ignoreFile;// addFIle -> add.log, ignoreFile -> .imperiumignore
	
	ignoreFile.open(root+"/.imperiumignore");
	addFIle.open(root+"/.imperium/add.log");
	if(ignoreFile.is_open()){// 
		while(!ignoreFile.eof()){
			getline(ignoreFile,file_or_dir);
			if(file_or_dir.back()=='/'){ file_or_dir.pop_back(); ignoreDir.push_back(root+"/"+file_or_dir);}
			// storing path for directories without / at end from .imperiumignore
			else ignorefilenames.push_back(root+"/"+file_or_dir);
		}
	}
	ignoreFile.close();
	if(onlyImperiumIgnore==0){// ?
		if(addFIle.is_open()){
			while(!addFIle.eof()){
				getline(addFIle,file_or_dir);
				int len=file_or_dir.length();
				if(len>4){//( ",",-,d/f) these 4 character is always there for ex - "/workspace/container_1"-d
					addedFileNames.pb({file_or_dir.substr(1,len-4),file_or_dir.back()});
				}
			}
		}
		addFIle.close();
		for(auto fileEntry:addedFileNames){
			if(addPath.compare(fileEntry.first)==0){// if present in add.log then update the file/folder
				addtoCache(addPath,fileEntry.second);//fileEntry.second -> type (f/d)
				cout<<"Updated "<<addPath<<endl;
				return true;
				//return true <- i.e, addPath is to be ignored.
			}
		}
	}
		if(find(ignorefilenames.begin(),ignorefilenames.end(),addPath)!=ignorefilenames.end()||ignoreFolder(addPath,ignoreDir)) return true;  
// check if addPath is present in ignorefilenames,ignoreDir vector,separate ignoreFolder as file can be nested in it.
else 
	return false;
}

void add(string &addPath){
	string checkPath=root+"/.imperium";
	struct stat buf;
	if(stat(checkPath.c_str(),&buf)) {
		cout<<"repository has not been initialized yet\n"; return;
	}
	ofstream addFIle;
	checkPath+="/add.log";
	struct stat buff2;
	if(stat(checkPath.c_str(),&buff2)) { ofstream addLog(checkPath.c_str()); addLog.close(); }// if not already, add it
	addFIle.open(root+"/.imperium/add.log",std::ios_base::app);// addFIle <- ofstream of add.log
	struct stat buffer;
	if(stat(addPath.c_str(),&buffer)!=0) {cout<<"ERROR: path doesnot exists.\n"; return;}

	if(buffer.st_mode & S_IFDIR) {// If the addPath is a directory
		if(!toBeIgnored(addPath)) {
			 addFIle<<"\""<<addPath<<"\""<<"-d\n";
			// write in add.log -d : identification for the path to be a directory
			 addtoCache(addPath,'d');
			 cout<<"Added directory "<<"\""<<addPath<<"\""<<endl;
		}
		for(auto &p: fsnew::recursive_directory_iterator(addPath))
		{
			if(toBeIgnored(p.path())) { continue;}
			struct stat buffer2;
			if(stat(p.path().c_str(),&buffer2)==0){
				if(buffer2.st_mode & S_IFDIR) {
					 addFIle<<p.path()<<"-d\n";
					 addtoCache(p.path(),'d');
					 cout<<"Added Directory "<<p.path()<<endl;
				}
				else if(buffer2.st_mode & S_IFREG){
					 addFIle<<p.path()<<"-f\n";
					 addtoCache(p.path(),'f');
					 cout<<"Added File "<<p.path()<<endl;
				}
				else continue;
			}
		}	
	}
	else if(buffer.st_mode & S_IFREG){
		if(toBeIgnored(addPath)) return;
		addFIle<<"\""<<addPath<<"\""<<"-f\n";
		addtoCache(addPath,'f');
		cout<<"Added File "<<addPath<<endl;
	}
	else cout<<"Error : Invalid Path\n";
	addFIle.close();
}
string timeHash(){
	auto timenow = chrono :: system_clock :: to_time_t(chrono::system_clock::now());
	unsigned char result [20] = {};
	SHA1((const unsigned char *)&timenow,sizeof(timenow),result);
	string ans;
	for(int i=0;i<20;i++){
		char temp; int val= sprintf(&temp,"%x",(unsigned int)result[i]); ans+=temp;
	}
	return ans;
}

//add file/folder to .commit/commithash
void addCommit(string path,char type,string commitHash)
{
	struct stat checkCommit;
	if(stat((root+"/.imperium/.commit").c_str(),&checkCommit)) //if .commit dont exists create it.
		mkdir((root+"/.imperium/.commit").c_str(),0777);
	string commitHashPath=root+"/.imperium/.commit/"+commitHash;
	struct stat commitHashCheck;
	if(stat(commitHashPath.c_str(),&commitHashCheck)){
		if(mkdir(commitHashPath.c_str(),0777)){
			cout<<"ERROR couldnot create commit hash folder\n"; 
			return;
		}
	}
	string filterPath=root+"/.imperium/";
	if(type =='d'){
		string filename=path.substr(filterPath.length());//relative path(w.r.t .imperium) of folder to commit
		bool flag=(filename[1]!='c');//check if path is from .commit folder -> means copying previous commit's content
		while(1) {
			if(filename[0]=='/'){ 
				if(flag) //if path was from .add folder then 
					break; 
				flag=1;
			}
			filename.erase(filename.begin());
		}
		string filerel=commitHashPath+filename;
		struct stat buffer;
		if(stat(filerel.c_str(),&buffer)) 
			fsnew::create_directories(filerel);
	}
	else if(type=='f'){
		string filename=path.substr(filterPath.length());//relative path(w.r.t .imperium) of file to commit
		bool flag=(filename[1]!='c');
		while(1) {
			if(filename[0]=='/'){
				if(flag)
					break; 
				flag=1;
			}
			filename.erase(filename.begin());
		}
		string filerel=commitHashPath+filename.substr(0,filename.find_last_of("/"));
		struct stat buffer2;
		if(stat(filerel.c_str(),&buffer2)) 
			fsnew::create_directories(filerel);
		fsnew::copy_file(path,commitHashPath+filename,fsnew::copy_options::overwrite_existing);
	}
}

//update the head commit and rewrite previous commit below it in commit.log
void updateCommitLog(string message,string commitHash){
	string commitLogPath=root+"/.imperium/commit.log";
	struct stat buffer;
	if(stat(commitLogPath.c_str(),&buffer)){// if commit.log does not exists create it
		ofstream commitLog(commitLogPath.c_str()); 
		commitLog.close();
	}
	ofstream tempCommitLog((root+"/.imperium/tempCommit.log").c_str());
	tempCommitLog<<commitHash<<" -- "<<message<<" --> HEAD\n";
	ifstream originalCommit; 
	originalCommit.open(commitLogPath);
	if(originalCommit.is_open()){
		bool flag=0; string line;
		while(!originalCommit.eof()){
			getline(originalCommit,line);
			if(!flag){ // if it is the first line -> previous hash which had --> head flag to it
				line=line.substr(0,line.length()-9); 
				flag=1; 
			}
			tempCommitLog<<line<<"\n";
		}
	}
	else {
		cout<<"Cannot open commit.log\n"; 
		return;
	}
	tempCommitLog.close(); 
	originalCommit.close();
	remove(commitLogPath.c_str());//delete commit.log
	rename((root+"/.imperium/tempCommit.log").c_str(),commitLogPath.c_str());//rename tmpCommit to commit.log
}

//return the hash of head
string getHead(){
	string commitLogPath= root + "/.imperium/commit.log";
	ifstream commitLog;
	commitLog.open(commitLogPath.c_str());
	if(commitLog.is_open()){
		while(!commitLog.eof()){
			string commitHash;
			getline(commitLog,commitHash);
			if(commitHash.size()<20) 
				continue;
			int index=-1;
			while(commitHash[++index]!=' ');//get string before -- message -> in commit.log
			commitHash=commitHash.substr(0,index);
			commitLog.close();
			return commitHash;
		}
	}
	else {cout<<"Could not open commit log\n"; exit(0);}
	return string("ncf");
}


void commit(char *argv[],int argc){
	struct stat checkImperium;
	if(stat((root+"/.imperium").c_str(),&checkImperium)) {
		cout<<"Repository has not been initialized yet\n"; 
		return;
	}
	if(strcmp(argv[2],"-m")) {
		cout<<"No message found, Please enter some message\n"; 
		return;
	}
	string message;// to store the message passed
	for(int i =3 ;i<argc;i++) {
		message+=argv[i]; 
		if(i<argc-1) message+=" "; 
	}
	string addPath=root+"/.imperium/.add";
	struct stat Buffer1; 
	if(stat(addPath.c_str(),&Buffer1)) {
		cout<<".add folder not created\n"; 
		return;
	}
	string commitHash=timeHash();
	bool addEmpty=1; //checks if .add is empty or not
	string lastCommit = getHead();
	for(auto &absPath:fsnew::recursive_directory_iterator(addPath)){
		string str=absPath.path();// inbuilt path() function in recursive_directory_iterator.
		if(str.length()<=addPath.length())
			continue;
		addEmpty=0; 
		break;
	}
	if(addEmpty) {
		cout<<"Nothing to commit\n"; 
		return;
	}
	if(lastCommit!="ncf")// if this is not the first commit copy content of last commit to recent commit
	{
			lastCommit=root+"/.imperium/.commit/"+lastCommit;
		for(auto &paths:fsnew::recursive_directory_iterator(lastCommit)){
			addEmpty=0;
			struct stat bufferLastCommit;
			if(stat(paths.path().c_str(),&bufferLastCommit)) {
				cout<<"some path does not exist\n"; 
				return;
			}
			string str=paths.path(); 
			str=str.substr(lastCommit.length());// relative path ,relative to .commit
			str=root+str; //file/folder relative root
			struct stat checkStr;
			if(stat(str.c_str(),&checkStr))//if file was deleted from the root after previous commit then dont commit
				continue;
			if(bufferLastCommit.st_mode & S_IFDIR) 
				addCommit(paths.path(),'d',commitHash);
			else if(bufferLastCommit.st_mode & S_IFREG) 
				addCommit(paths.path(),'f',commitHash);
		}
	}
	// now commiting changes from .add 
	for(auto &absPath:fsnew::recursive_directory_iterator(addPath))
	{
		struct stat buffer;
		if(stat(absPath.path().c_str(),&buffer)){
			cout<<"ERROR path doesnot exist\n"; 
			return;
		}
		char type;
		if(buffer.st_mode & S_IFDIR) 
			type='d';
		else if(buffer.st_mode & S_IFREG) 
			type='f';
		else continue;
		addCommit(absPath.path(),type,commitHash);
	}
	fsnew::remove_all(addPath.c_str());//deleting content of .add folder as file/folders have been commited
	mkdir(addPath.c_str(),0777);
	addPath=addPath.substr(0,addPath.find_last_of("/")); 
	addPath+="/add.log";
	remove(addPath.c_str());//deleting entries of add.log
	ofstream addLog(addPath.c_str());
	addLog.close();
	if(!addEmpty)
		updateCommitLog(message,commitHash);
}

//function to list content of commit.log -> all commits taken place
void log(){
	string path = root+"/.imperium/commit.log";
	struct stat buffer;
	if(stat(path.c_str(),&buffer)) {
		 cout<<"Commit log not created yet\n"; 
		 return;
	}
	ifstream commitLog;
	commitLog.open(path.c_str());
	if(commitLog.is_open()){
		string line;
		while(!commitLog.eof()){
			getline(commitLog,line);
			if(line.size()<20) continue;
			cout<<line<<endl;
		}
	}
	else cout<<"Could not open commit log\n";
	commitLog.close();
}

//function to retrieve particular commithash(path) to root(either overwrites or copies its content back to root)
void addCheckout(string path,char type){
	if(type=='d'){
		string parentPath = root + "/.imperium/.commit/";
		path = path.substr(parentPath.length()); // relative path w.r.t .commit
		int index=-1;
		while(path[++index]!='/'); // "/" just after commit hash for ex - c5e3a61e2a7bad113d4d/script/
		path = path.substr(index+1);
		string destPath = root + "/" +path; // go to the same file/folder which is in the root(if exists)
		struct stat buffer;
		if(stat(destPath.c_str(),&buffer) != 0){
			fsnew::create_directories(destPath); //create a new directory in the root if it does not exists
		}
	}
	else if(type == 'f'){
		string parentPath = root + "/.imperium/.commit/";
		string relpath = path.substr(parentPath.length());
		int index=-1;
		while(relpath[++index]!='/');
		relpath = relpath.substr(index);
		string destDir = root + relpath.substr(0,relpath.find_last_of("/"));
// desdir -> folder in which the file exists in .commit(just parent folder) which is to be copied/overwriten in root
		struct stat buffer2;
		if(stat(destDir.c_str(),&buffer2)) 
			fsnew::create_directories(destDir);
		destDir = root + relpath;
		fsnew::copy_file(path,destDir,fsnew::copy_options::overwrite_existing);
		// copy/overwrite from path to desDir
	}
}

//check if commithash exists, if it does copy/overwrite file/folder in the root
void checkout(string commitHash){
	string commitPath = root + "/.imperium/commit.log";
	ifstream commitLog(commitPath.c_str());
	bool commitExists=0;
	if(commitLog.is_open()){
		while(!commitLog.eof()){
			string line; getline(commitLog,line);
			int index=-1;
			while(line[++index]!=' ');
			line=line.substr(0,index);// abstracting commitHash
			if(line==commitHash){ commitExists=1; break;}
		}
	}
	else {cout<<"Could not open commit log\n"; return;}
	commitLog.close();
	if(commitExists){
		commitPath=root+"/.imperium/.commit/"+commitHash;
		for(auto &filePath:fsnew::recursive_directory_iterator(commitPath)){
			struct stat filebuffer;
			if(stat(filePath.path().c_str(),&filebuffer)==0){
				if(filebuffer.st_mode & S_IFDIR){
					addCheckout(filePath.path(),'d');
				}
				else if(filebuffer.st_mode & S_IFREG){
					addCheckout(filePath.path(),'f');
				}
			}
		}
		cout<<"Checkout successful\n";
	}
	else cout<<"No commits found\n";
}

// returns 1 if lFilePath and rFilePath are different else 0
bool comparefiles(string lFilePath,string rFilePath){
	int BUFFER_SIZE =1;
	std::ifstream lFile(lFilePath.c_str(), std::ifstream::in | std::ifstream::binary);
    std::ifstream rFile(rFilePath.c_str(), std::ifstream::in | std::ifstream::binary);

    if(!lFile.is_open() || !rFile.is_open())
    {
        return 1;
    }

    char *lBuffer = new char[BUFFER_SIZE]();
    char *rBuffer = new char[BUFFER_SIZE]();

    do {
        lFile.read(lBuffer, BUFFER_SIZE);
        rFile.read(rBuffer, BUFFER_SIZE);

        if (std::memcmp(lBuffer, rBuffer, BUFFER_SIZE) != 0)
        {
            delete[] lBuffer;
            delete[] rBuffer;
            return 1;
        }
    } while (lFile.good() || rFile.good());

    delete[] lBuffer;
    delete[] rBuffer;
    return 0;
}

// prints the status of the staged and unstaged files 
// either they are staged/unstaged -> 0 - created, 1 - modified, 2 - unchanged/unmodified
// in staged only check w.r.t previous commit not root
// staged created --> if this file/folder does not exists in previous commit
// staged unchanged --> only if same with previous commit - no need to see the root
// staged modified --> only different w.r.t previous commit, not root.
// unstaged unchanged --> only when same with previous commit and staged(or do not exists in it)
// otherwise unstaged modified.
// unstaged created --> if this file/folder does not exist in .dd and .commit both
void status(){
	vector<string> staged[3]; vector<string> unstaged[3];
	string commitHash=getHead();
	string addLogPath = root + "/.imperium/add.log";
	ifstream addLog;
	addLog.open(addLogPath.c_str());
	if(addLog.is_open()){
		while(!addLog.eof()){
			string filePath;
			getline(addLog,filePath);
			if(filePath.length()<4) 
				continue;
			filePath.erase(filePath.begin());// erasing " -> of beginning 
			filePath.erase(filePath.begin()+filePath.length()-3,filePath.end());// erasing " -d or " -f
			//now filepath contains path of folder in root (which is in add.log)
			filePath=filePath.substr(root.length());// relative path w.r.t root
			string commitPath = root + "/.imperium/.commit/";
				struct stat buffer;
				if(stat((commitPath+commitHash+filePath).c_str(),&buffer)==0){
					// if that file/folder has been commited
					string path1=root + "/.imperium/.add"+filePath;// file/folder in .add
					string path2=commitPath+commitHash+filePath;// file/folder in .commit
					string path3= root + filePath;// file/folder in root
					if(comparefiles(path1,path2)){// if respective file in .add and .commit is different
						string pushin = root+filePath;
						staged[1].pb(pushin); //then it is STAGED modified --> means modified w.r.t previous commit
					}
					else {
						string pushin= root+filePath;
						staged[2].pb(pushin); // else staged unmodified
					}
					if(comparefiles(path1,path3)){// if respective file in .add and root is different
						struct stat buffer2;
						if(stat(path1.c_str(),&buffer2)) 
							continue;
						string pushin=root+filePath;
						unstaged[1].pb(pushin);// then it is UNSTAGED modified --> means modified w.r.t staged 
					}
				}
				else{
					string pushin=root+filePath;
					staged[0].pb(pushin); //then it is STAGED created as it has never been commited
				}
		}
		for(auto &p:fsnew::recursive_directory_iterator(root)){
			if(toBeIgnored(p.path(),1)) continue;
			// same with add / add is not present
			string commitPath=p.path();
			if(commitPath.length()<=root.length()) continue;
			commitPath=commitPath.substr(root.length());
			string relpath=commitPath;
			string addPath = root + "/.imperium/.add/"+relpath;
			struct stat bufbuf;  bool sameWithAdd = 0;
			if(stat(addPath.c_str(),&bufbuf)==0){
				sameWithAdd = 1-comparefiles(root+relpath,addPath); 
				if(!sameWithAdd) // if different from file/folder in .add then continue as handled previously
					continue;
			}
			//now either the file/folder in .add does not exist or is same as root
			commitPath=root+"/.imperium/.commit/"+commitHash+commitPath; // file in commit folder
			// last commit doesn't exists
			struct stat commitPathBuffer;
			bool commitPathExists = (stat(commitPath.c_str(),&commitPathBuffer)==0); // if this file exists
			if(commitPathExists){//if file/folder has been commited
				if(comparefiles(root+relpath,commitPath)) unstaged[1].pb(root+relpath);//then unstaged modified
				else unstaged[2].pb(root+relpath); // either it is same with file/folder in add or add doesn't exist
			}
			else{
				if(sameWithAdd) unstaged[2].pb(root+relpath);
				else unstaged[0].pb(root+relpath); // file and folder in .add and .commit does not exists
			}
		}
	}
	else {cout<<"Could not open add log\n"; return;}
	addLog.close();
	cout<<"Staged : \n";
	cout<<"Created files : \n\n";
	if(staged[0].empty()) cout<<"Empty\n";
	for(string files:staged[0]) cout<<"---> "<<files<<endl;
	cout<<"\nModified Files : \n\n";
	if(staged[1].empty()) cout<<"Empty\n";
	for(string files:staged[1]) cout<<"---> "<<files<<endl;
	cout<<"\nUnchanged Files : \n\n";
	if(staged[2].empty()) cout<<"Empty\n";
	for(string files:staged[2]) cout<<"---> "<<files<<endl;
	cout<<"\nUnstaged : \n";
	cout<<"Created files : \n\n";
	if(unstaged[0].empty()) cout<<"Empty\n";
	for(string files:unstaged[0]) cout<<"---> "<<files<<endl;
	cout<<"\nModified Files : \n\n";
	if(unstaged[1].empty()) cout<<"Empty\n";
	for(string files:unstaged[1]) cout<<"---> "<<files<<endl;
	cout<<"\nUnchanged Files : \n\n";
	if(unstaged[2].empty()) cout<<"Empty\n";
	for(string files:unstaged[2]) cout<<"---> "<<files<<endl;
	cout<<endl;
}


void revert(string passedHash){
	string headHash = getHead();
	vector<string> conflict;// to store the path of conflict files/folder
	if(headHash=="ncf") { // no commits yet
		cout<<"No commits found\n"; 
		return;
	}
	for(auto &p:fsnew::recursive_directory_iterator(root)){
		string path=p.path();
		struct stat buffer;
		if(stat(path.c_str(),&buffer)==0){// if the file/folder in the root exists
			if(buffer.st_mode & S_IFREG){
				string filerel = path.substr(root.length());
				string checkPath = root + "/.imperium/.commit/"+headHash+filerel;
				struct stat buffer1;
				// if a file/folder exists in root but not in latest commit -> it is created in root, so it need not be changed.
				if(stat(checkPath.c_str(),&buffer1)) 
					continue;
				// if file/folder exists in latest commit but is different with that in root
				if(comparefiles(path,checkPath)) {
					cout<<"please commit latest changes first\n"; 
					return;
				}
			}
		}
	}
	// so till now we have all file/folder's content similar with the latestcommit(headhash) or file that is created in root.
	string lastHash; 
	ifstream commitLog;
	commitLog.open(root+"/.imperium/commit.log");
	bool flag=0;
	if(commitLog.is_open()){// if commitlog exists
		while(!commitLog.eof()){
			string temp;
			getline(commitLog,temp);
			if(temp.size()<20) continue;
			lastHash=temp;
			int index=-1;
			while(lastHash[++index]!=' ');
			lastHash = lastHash.substr(0,index);
			if(flag) // if we have visited passedhash
				break;
			if(lastHash==passedHash) flag=1;
		}
	}
	else {
		cout<<"Could not open commit log\n"; 
		return;
	}
	commitLog.close();
	if(flag == 0) {// hash passed in invalid
		cout<<"Invalid hash passed\n"; 
		return;
	}
	if(lastHash==passedHash) {// if passedHash is first commit
		for(auto &p:fsnew::recursive_directory_iterator(string(root + "/.imperium/.commit/"+passedHash))){
			string path = p.path();
			struct stat buffer; 
			string parent = root + "/.imperium/.commit/" + passedHash;
			string filerel = path.substr(parent.length());// relative path w.r.t passedhash
			if(stat(path.c_str(),&buffer)==0) {// if file/folder exists in passedHash commit
				if(buffer.st_mode & S_IFREG){ // if it is a file
					string checkPath = root + "/.imperium/.commit/"+headHash+filerel;//checking if the file exists in headhash
					if(comparefiles(path,checkPath)){// if both file(headhash and passedhash) are different -> merge conflict
						ofstream mergeConflict;
						mergeConflict.open(root+filerel,std::ios_base::app);
						// open the file in root and append it with conflict error
						mergeConflict<<"This file was created in the reverted commit and subsequent changes have been preserved\n";
						conflict.pb(root+filerel);
						mergeConflict.close();
					}
					//if both file(headhash and passedhash) are same and if not found in conflict vector,  i.e. the file does 						//not have merge conflict so we can delete it
					else if(find(conflict.begin(),conflict.end(),root+filerel)==conflict.end()) {
						string deletePath = root + filerel;
						//delete the file
						if(remove(deletePath.c_str())) {
							cout<<"Error in deleting files\n"; 
							return;
						}
					}
				}
				else if(buffer.st_mode & S_IFDIR) // if it is a folder
					addCheckout(path,'d');
				// retrieve passedhash's(path) content to root(either overwrites or copies its content back to root)
			}
		}
	}
	else if(passedHash==headHash){// if passedhash is the latest commit
		//retrieve lasthash's(path) content to root(either overwrites or copies its content back to root)
		for(auto &p:fsnew::recursive_directory_iterator(string(root+"/.imperium/.commit/"+lastHash)))
		{
			string path = p.path();
			struct stat buffer;
			if(stat(path.c_str(),&buffer)==0){
				if(buffer.st_mode & S_IFDIR){
					addCheckout(path,'d');
				}
				else if(buffer.st_mode & S_IFREG){
					addCheckout(path,'f');
				}
			}
		}
		// Now recursively go in root and delete the files/folder which are not present in lastHash and present in headhash
		for(auto &p:fsnew::recursive_directory_iterator(root)){
			string path = p.path();
			string filerel = path.substr(root.length());// relative path of file/folder w.r.t root
			struct stat buffer; 
			if(stat(path.c_str(),&buffer)==0){ // if path exists
				if(buffer.st_mode & S_IFDIR) continue;
			}
			string path1 = root + "/.imperium/.commit/"+lastHash+filerel;
			string path2 = root + "/.imperium/.commit/"+headHash+filerel;
			struct stat buffer1;
			if(stat(path1.c_str(),&buffer1)){// if file/folder does not exists in lastHash
				struct stat buffer2;
				if(stat(path2.c_str(),&buffer2)==0) // if file/folder does not exists in lastHash and exists in headhash
					remove(path.c_str()); // delete the file/folder
			}
		}
	}
	else{ // if passedhash hash is neither headhash nor firsthash(1st commit)
		for(auto &p:fsnew::recursive_directory_iterator(string(root+"/.imperium/.commit/"+lastHash)))
		{
			string path = p.path();
			string parent = root+"/.imperium/.commit/"+lastHash;
			string filerel =path.substr(parent.length());// relative path w.r.t parent(lasthash)
			struct stat buffer;
			if(stat(path.c_str(),&buffer)==0){// if file/folder exists in lasthash
				if(buffer.st_mode & S_IFREG) {// if it is a file
					string checkPath = root + "/.imperium/.commit/"+headHash+filerel;
					struct stat buffer2;
					if(stat(checkPath.c_str(),&buffer2)==0){// if file exists in headhash
						if(comparefiles(path,checkPath)){// if both files are different -> merge conflict
							ofstream mergeConflict;
							mergeConflict.open(root+filerel,std::ios_base::app);
							mergeConflict<<"Merge conflict, current file has been updated. Preserving changes.\n";
							conflict.pb(root+filerel);// push the path in conflict vector, to know if this file is conflicted
							mergeConflict.close();
						}
						else // if both files are same
							addCheckout(path,'f');
					}
					else 
						addCheckout(path,'f');
				}
			}
		}
		for(auto &p:fsnew::recursive_directory_iterator(string(root+"/.imperium/.commit/"+passedHash))){
			string path = p.path();
			string parent  = root+"/.imperium/.commit/"+passedHash;
			string filerel = path.substr(parent.length());
			struct stat buffer;
			if(stat(path.c_str(),&buffer)==0){// if file/folder present in passedHash
				if(buffer.st_mode & S_IFREG){// if it is a file
					string checkPath = root + "/.imperium/.commit/"+headHash+filerel;
					struct stat buffer2;
					if(stat(checkPath.c_str(),&buffer2)==0){// if the same file exists in headhash
						if(comparefiles(path,checkPath)){// if both are different -> merge conflict
							checkPath=root + "/.imperium/.commit/"+lastHash+filerel;
							struct stat buffer3;
							// if that file does not exists in lasthash -> it was created in passedhash -> merge conflict
							if(stat(checkPath.c_str(),&buffer3)) 
							{
								ofstream mergeConflict;
								mergeConflict.open(root+filerel,std::ios_base::app);
								mergeConflict<<"This file was created in the reverted commit and subsequent changes have been preserved\n";
								conflict.pb(root+filerel);
								mergeConflict.close();
							}
						}
						//if not in conflict vector and file are same(last and head) delete it from root
						else if(find(conflict.begin(),conflict.end(),root+filerel)==conflict.end()) 		  					       						 remove(string((root+filerel)).c_str()); 
					}
				}
				else if(find(conflict.begin(),conflict.end(),root+filerel)==conflict.end())                                                         remove(string((root+filerel)).c_str());
			}
		}
	}
	if(conflict.size()>0){// if there are some merge conflicts
		cout<<"Merge conflicts found with : \n";
		string conflictPath = root + "/.imperium/conflict.log";
		remove(conflictPath.c_str());
		ofstream conflictLog(conflictPath.c_str());//truncating conflict.log content to true
		conflictLog<<"True\n";
		for(string s:conflict){// register all conflict files in conflict.log
			cout<<s<<endl;
			conflictLog<<s<<"\n";
		}
		conflict.clear();
		conflictLog.close();
	}
}
// return true if there are conflicting files
bool conflict(){
	string conflictPath=root+"/.imperium/conflict.log";
	ifstream conflictLog;
	conflictLog.open(conflictPath);
	if(conflictLog.is_open()){
		string line;
		if(conflictLog.eof()) {cout<<"Conflict log empty\n"; exit(0);}
		else getline(conflictLog,line);
		return (line=="True");
	}
}
// just mention false(i.e no more conflict now) in conflict.log 
void resolve(){
	string conflictPath=root+"/.imperium/conflict.log";
	remove(conflictPath.c_str());
	ofstream conflictLog(conflictPath.c_str());
	conflictLog<<"False\n";
	conflictLog.close();
	cout<<"Resolved !!\n";
	exit(0);
}

void branch(string branchName){
	string newRoot = "/workspace/"+branchName;
	struct stat checkBranch;
	if(stat(newRoot.c_str(),&checkBranch)==0) {cout<<"Branch alreay exists\n"; return;}
	mkdir(newRoot.c_str(),0777);
	string master= "/workspace/10DaysOfCode";
	ofstream branchLog;
	branchLog.open(master+"/.imperium/branch.log",std::ios_base::app);
	if(branchLog.is_open()){
		branchLog<<newRoot<<" created\n";
	}
	else {
		cout<<"Could not open branch log in master\n"; 
		return;
	}
	branchLog.close();
	string originalMake="/workspace/10DaysOfCode/Makefile";
	string copyMake = "/workspace/"+branchName+"/Makefile";
	cout<<"Created branch "<<branchName<<endl;
	fsnew::copy_file(originalMake,copyMake);
	root = newRoot;
	init();
}

bool ifIgnored(string path){
	string prefix = "/workspace/";
	path=path.substr(prefix.length());
	string branch=path.substr(0,path.find_first_of("/")); path=path.substr(0,path.find_first_of("/"));
	branch = prefix + branch;
	ifstream ignoreFile;
	vector<string> filenames;
	vector<string> ignoreDir;
	ignoreFile.open(branch+"/.imperiumignore");
	if(ignoreFile.is_open()){
		while(!ignoreFile.eof()){
			string file_or_dir; getline(ignoreFile,file_or_dir);
			if(file_or_dir.back()=='/') ignoreDir.pb(branch+"/"+file_or_dir);
			else filenames.pb(branch+"/"+file_or_dir);
		}
	}
	else {cout<<"Could not open imperiumignore\n"; exit(0);}
	ignoreFile.close();
	if(find(filenames.begin(),filenames.end(),branch+path)!=filenames.end()||ignoreFolder(path,ignoreDir)){
		return true;
	}
	else return false;
}

void merge(string fromBranch,string toBranch="",bool master=1){
	fromBranch="/workspace/"+fromBranch;
	struct stat checkFromBranch;
	if(stat(fromBranch.c_str(),&checkFromBranch)!=0) {cout<<"No such branch\n"; return;}
	string branchName = "master";
    if(!master){
		toBranch="/workspace/"+toBranch;
		struct stat checkCorrect;
		if(stat(toBranch.c_str(),&checkCorrect)!=0) {cout<<"No such branch found\n"; return;}
		branchName = toBranch.substr(toBranch.find_last_of("/")+1);
	}
	string branchLogPath = "/workspace/10DaysOfCode/.imperium/branch.log";
	ofstream branchLog;
	branchLog.open(branchLogPath,std::ios_base::app);
	if(!branchLog.is_open()) {cout<<"Could not open branch log\n"; return;}
	branchLog<<fromBranch<<" merged with "<<(master?"master":toBranch)<<endl;
	branchLog.close();
	string temproot=fromBranch; root = "/workspace/10DaysOfCode"; if(!master) root = toBranch;
		for(auto &p:fsnew::recursive_directory_iterator(temproot)){
			if(ifIgnored(p.path())) continue;
			struct stat buffer;
			if(stat(p.path().c_str(),&buffer)==0){
				if(buffer.st_mode & S_IFDIR){
					string relpath=p.path();
					relpath=relpath.substr(temproot.length());
					if(relpath.empty()) continue;
					relpath=root+relpath;
					struct stat buffer2;
					if(stat(relpath.c_str(),&buffer2)!=0) fsnew::create_directories(relpath);
				}
				else if(buffer.st_mode & S_IFREG){
					string originalPath = p.path();
					string relpath=p.path();
					relpath=relpath.substr(temproot.length());
					relpath=root+relpath;
					string filename = relpath.substr(relpath.find_last_of("/")+1);
					relpath = relpath.substr(0,relpath.find_last_of("/"));
					struct stat buffer2;
					if(stat(relpath.c_str(),&buffer2)!=0) fsnew::create_directories(relpath);
					fsnew::copy_file(originalPath,relpath+"/"+filename,fsnew::copy_options::overwrite_existing);
				}
			}
		}
	cout<<"Merged successfully\n";
	fsnew::remove_all(fromBranch.c_str());
}

void listBranches(){
	ifstream branchLog;
	branchLog.open("/workspace/10DaysOfCode/.imperium/branch.log");
	if(!branchLog.is_open())  {cout<<"Could not open branch log\n"; return;}
	unordered_set<string> branches;
	while(!branchLog.eof()){
		string line; getline(branchLog,line);
		string status = line.substr(line.find_first_of(" ")+1);
		if(status=="created"){
			string branch = line.substr(0,line.find_first_of(" "));
			branches.insert(branch);
		}
		else{
			string branch = line.substr(0,line.find_first_of(" "));
			branches.erase(branch);
		}
	}
	if(branches.empty()){
		cout<<"No branches other than master\n"; return;
	}
	cout<<"Current open branches(other than master):\n";
	for(string branch : branches) cout<<branch<<endl;
	cout<<endl;
}
void help(string command){
	if(command == "init"){
		cout<<"Initialises a repository that contains a setup of files, needed for performing all other commands on inperium\nThis is the first command you should give in when using imperium\n";
	}
	else if(command == "add"){
		cout<<"This is the second step of using version control.\nThis command is given in format imperium add [path]\n";
		cout<<"This adds the file(s) present in [path] to the staging are, where they ready to get saved.\n";
	}
	else if(command == "commit"){
		cout<<"This command is one of the most important command in our version control.\n";
		cout<<"Commit saves the files that were staged using add.\n";
		cout<<"It is passed as imperium commit -m message. Where message is written by the user.\n";
	}
	else if(command == "log"){
		cout<<"This command lists all the commits made by user, and the latest commit denoted by a \"HEAD\" tag.\n";
	}
	else if(command == "checkout"){
		cout<<"This command is kind-of time travel command. It loads a previously saved commit.\n";
		cout<<"Along with this command, the commit hash of the commit is also to be passed. Commit hash can be identified by log.\n";
	}
	else if(command == "revert"){
		cout<<"This command reverts back changes made by the passed commit.";
		cout<<"Commit hash is passed along with this commit.\n";
	}
	else if(command == "resolve"){
		cout<<"This command is used to \"unfreeze\" all the commands frozen by a merge conflict.\n";
	}
	else if(command == "new"){
		cout<<"Creates a new branch.\n";
		cout<<"Command is passed as imperium new <branch name>\n";
		cout<<"Commadn can be called from any branch.\n";
	}
	else if(command == "merge"){
		cout<<"Merges two branches.\n";
		cout<<"Command is given as imperium merge branch1 branch2\n";
		cout<<"Merges branch1 into branch2, deleting branch1.\n";
		cout<<"If branch2 is not specified, it's the master branch by default.\n";
		cout<<"This command can only be called from the master branch.\n";
	}
	else if(command == "list"){
		cout<<"Lists all active branches, other than master.\n";
	}
	else if(command == "help"){
		cout<<"Are u kiddin me -_-\n";
	}
	else cout<<"Enter a valid command.\n";
}
int main(int argc,char* argv[]) {
	if(argc==1) {cout<<"Imperium Commands :\n";
	cout<<"init\nadd\ncommit\nlog\ncheckout\nstatus\nrevert\nresolve\nnew\nmerge\nlist\ntype help <command> to know about a specific command\n";
	return 0;}
	root=getenv("dir");
	if(conflict()) {// if there are conflict files then resolve
		if(strcmp(argv[1],"resolve")==0) resolve();
		cout<<"ERROR, existing merge conflicts\n"; return 0;
	}
	if(strcmp(argv[1],"init")==0)
	{
        init(root);
	}
	else if(strcmp(argv[1],"add")==0){
		string addPath=root; 
		if(strcmp(argv[2],".")) {
			addPath+="/"; 
			addPath+=argv[2];
		}
		add(addPath);
	}
	else if(strcmp(argv[1],"commit")==0){
		if(argc<4) {cout<<"invalid commit\n"; return 0;}
		commit(argv,argc);
	}
	else if(strcmp(argv[1],"log")==0){
		log();
	}
	else if(strcmp(argv[1],"checkout")==0){
		if(argc<3) {cout<<"Please enter a commit hash\n"; return 0;}
		string commitHash=argv[2];
		checkout(commitHash);
	}
	else if(strcmp(argv[1],"status")==0){
		status();
	}
	else if(strcmp(argv[1],"revert")==0){
		if(argc<3) {cout<<"Please enter a revert hash\n"; return 0;}
		string revertHash=argv[2]; 
		revert(revertHash);
	}
	else if(strcmp(argv[1],"resolve")==0){
		resolve();
	}
	else if(strcmp(argv[1],"new")==0){
		if(argc<3) {cout<<"Enter a branch name!!\n"; return 0;}
		if(argc>3) {cout<<"Branch name cannot contain spaces.\n"; return 0;}
		string branchName=argv[2];
		branch(branchName);
	}
	else if(strcmp(argv[1],"merge")==0){
		if(root!="/workspace/10DaysOfCode") {cout<<"Please change to master branch before merging\n"; return 0;}
		if(argc==2){
			cout<<"Enter a branch name!!\n"; return 0;
		}
		if(strcmp(argv[2],"10DaysOfCode")==0){
			cout<<"Cannot merge master branch into other branch.\n"; return 0;
		}
		if(argc==3){
			string branchName = argv[2];
			merge(branchName);
			return 0;
		}
		if(argc==4) {
			string fromBranch = argv[2];
			string givenbranch = argv[3];
			merge(fromBranch,givenbranch,0);
		}
		else cout<<"Can only merge two branches at a time\n";
	}
	else if(strcmp(argv[1],"list")==0){
		listBranches();
	}
	else if(strcmp(argv[1],"help")==0){
		if(argc==2) {
			cout<<"Imperium is an indigenous Version Control System(like git)\n";
			cout<<"Type help <command> for knowing about specific command\n";
			return 0;
		}
		string helpCommand = argv[2];
		help(helpCommand);
	}
	else cout<<"Enter a valid command\ntype imperium for list of availible commands\n";
	return 0;
}