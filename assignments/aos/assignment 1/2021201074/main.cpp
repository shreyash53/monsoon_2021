#include <bits/stdc++.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include<sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#define esc 27
#define cls printf("%c[2J", esc)

#define KEY_UP 65
#define KEY_DOWN 66
#define KEY_LEFT 68
#define KEY_RIGHT 67
#define KEY_ENTER 10
#define KEY_BACKSPACE 127

using namespace std;
struct termios old_terminal;
struct termios new_terminal;

#define highlight_green(message) cout << "\033[0;32m"; cout << message; cout << "\033[0m";


void enter_raw_mode()
{
	tcgetattr(STDIN_FILENO, &old_terminal);
	new_terminal = old_terminal;
	new_terminal.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_terminal);//changes will occur after flush
}

#define exit_raw_mode tcsetattr(STDIN_FILENO, TCSANOW, &old_terminal); //changes will occur now

#define moveCursor(x, y) cout << "\033[" << x << ";" << y << "H";	//x is row and y is coloumn

#define clear_line cout << "\033[K";

#define arrow moveCursor(win.currentScreenLineAt, 1); highlight_green("=>");



//from here


string currentDir;

struct window {


	int windowSizeRow;
	int windowSizeColumn;
	int currentScreenLineAt;
	int currentFileLineAt;
	int directionWindowTop;
	int directionWindowbottom;
	int directionWindowLeft;
	int directionWindowRight;


} win;

bool isOnLastLine() {
	return win.currentScreenLineAt == win.windowSizeRow - 1;
}

void openFile(string path) {
	pid_t processID = fork();
	if (processID == 0) {
		execlp("xdg-open", "xdg-open", path.c_str(), NULL);
		exit(0);
	}
}

struct file_meta {
	string name;
	off_t  size;
	mode_t mode;
	time_t mtime;

};

vector<file_meta> files;


struct directoryList {
	vector<string> history;
	int currentAt;
	directoryList() {
		currentAt = -1;
	}
	void insert(string path) {
		history.push_back(path);
		currentAt++;
	}

	bool empty() {
		return currentAt == -1;
	}

	string getCurrent() {
		return history[currentAt];
	}

	string getPrevious() {
		if (currentAt - 1 > -1)
			return history[--currentAt];
		return "";
	}

	string getNext() {
		if (currentAt + 1 < history.size())
			return history[++currentAt];
		return "";
	}
} dirList;


bool isDefaultFile(string name) {
	return name == "." or name == "..";
}

bool isNotValid(string nameFile, struct stat& sb) {
	return stat(nameFile.c_str(), &sb) == -1;
}

bool isItFile(struct stat& sb) {
	return (sb.st_mode & S_IFDIR) == 0;
}

bool isItFile(mode_t& fileMode) {
	return (fileMode & S_IFDIR) == 0;
}

void makeFileObject(string filename, string name, vector<file_meta>& foldersList, vector<file_meta>& filesList) {
	file_meta fm;
	struct stat sb;
	if (isNotValid(filename, sb)) {
		perror("stat() error");
		return;
	}

	fm.name = name;
	fm.size = sb.st_size;
	fm.mode = sb.st_mode;
	fm.mtime = sb.st_mtime;

	if (isItFile(fm.mode))
		filesList.push_back(fm);
	else
		foldersList.push_back(fm);
}


void defaultFileRead(string& path, vector<file_meta>& foldersList, vector<file_meta>& filesList) {
	makeFileObject(path + "/" + ".", ".", foldersList, filesList);

	makeFileObject(path + "/" + "..", "..", foldersList, filesList);
}

vector<file_meta> readAllDirs(string path) {
	vector<file_meta> foldersList;
	vector<file_meta> filesList;
	DIR* streamp = opendir(path.c_str());
	struct dirent *dep;

	defaultFileRead(path, foldersList, filesList);

	while ((dep = readdir(streamp)) != NULL) {
		if (isDefaultFile(dep -> d_name))
			continue;
		makeFileObject(path + "/" + dep->d_name, dep->d_name, foldersList, filesList);
	}
	closedir(streamp);


	for (auto fm : filesList)
		foldersList.push_back(fm);

	return foldersList;
}

void displayLine(string line) {
	for (int i = win.directionWindowLeft; i < line.length() and i <= win.directionWindowRight; i++) {
		cout << line[i];
	}
}

static void sig_winch(int signo) {
	struct winsize size;
	if (ioctl(STDIN_FILENO, TIOCGWINSZ, (char *) &size) < 0)
		cerr << "TIOCGWINSZ error";
	win.windowSizeRow = size.ws_row;
	win.currentScreenLineAt = 1;
	win.currentFileLineAt = 0;
	win.windowSizeColumn = size.ws_col;
	win.directionWindowTop = 1;
	win.directionWindowbottom = win.windowSizeRow - 1;
	win.directionWindowRight = win.windowSizeColumn - 4;
	win.directionWindowLeft = 0;
	cls;
}

void display_footer() {
	moveCursor(win.windowSizeRow, 0);
	clear_line;
	displayLine("Normal Mode:");
}

void displayFooterCommandModeText(string input) {
	displayLine("Command Mode:" + input);
}

void displayFooterCommandMode(string input) {
	moveCursor(win.windowSizeRow, 0);
	clear_line;
	displayFooterCommandModeText(input);
}



string mode_to_text(mode_t mode) {
	string res = "";
	if (mode & S_IFDIR)
		res += 'd';
	else
		res += '-';
	if (mode & S_IRUSR)
		res += 'r';
	else
		res += '-';
	if (mode & S_IWUSR)
		res += 'w';
	else
		res += '-';
	if (mode & S_IXUSR)
		res += 'x';
	else
		res += '-';
	if (mode & S_IRGRP)
		res += 'r';
	else
		res += '-';
	if (mode & S_IWGRP)
		res += 'w';
	else
		res += '-';
	if (mode & S_IXGRP)
		res += 'x';
	else
		res += '-';
	if (mode & S_IROTH)
		res += 'r';
	else
		res += '-';
	if (mode & S_IWOTH)
		res += 'w';
	else
		res += '-';
	if (mode & S_IXOTH)
		res += 'x';
	else
		res += '-';

	return res;
}


void displayAllDirs() {
	for (int i = win.directionWindowTop, j = 1; i <= files.size() and i <= win.directionWindowbottom; i++, j++) {
		moveCursor(j, 4);
		clear_line;
		displayLine(
		    mode_to_text(files[i - 1].mode) + " "
		    + to_string(files[i - 1].mtime) + " "
		    + to_string(files[i - 1].size) + " " +
		    files[i - 1].name);
	}
}

void resetScreen(string path, bool flag) {
	files = readAllDirs(path);
	currentDir = path;
	win.currentScreenLineAt = 1;
	win.currentFileLineAt = 0;
	win.directionWindowTop = 1;
	win.directionWindowbottom = win.windowSizeRow - 1;
	if (flag)
		dirList.insert(path);
}

void normalMode(char ch, int& mode) {
	if (ch == KEY_UP) //move up
	{
		moveCursor(win.currentScreenLineAt, 0);

		if (win.currentScreenLineAt == 1) {
			if (win.directionWindowTop > 1)
				win.directionWindowbottom--, win.directionWindowTop--;
		}
		else
			win.currentScreenLineAt--;

		clear_line;
		if (win.currentFileLineAt > 0)
			win.currentFileLineAt--;

	}
	else if (ch == KEY_DOWN) //move down
	{
		moveCursor(win.currentScreenLineAt, 0);

		if (isOnLastLine()) {
			if (win.directionWindowbottom < files.size())
				win.directionWindowTop++, win.directionWindowbottom++;
		}
		else {
			win.currentScreenLineAt++;
		}

		clear_line;

		if (win.currentFileLineAt < files.size() - 1)
			win.currentFileLineAt++;
	}
	else if (ch == KEY_ENTER) {
		cerr << files[win.currentFileLineAt].name << ", fl = " << win.currentFileLineAt << ", cl = " << win.currentScreenLineAt << ", bot = " << win.directionWindowbottom << ", row = " << win.windowSizeRow << endl;
		bool fileFlag = isItFile(files[win.currentFileLineAt].mode);

		string nextDirectory = currentDir + "/" + files[win.currentFileLineAt].name;
		cerr << nextDirectory << endl;
		if (!fileFlag and files[win.currentFileLineAt].name != ".") {
			struct stat sb;
			if (isNotValid(nextDirectory, sb)) {
				cerr << "error: invalid directory => " << nextDirectory << endl;
			}
			else {
				resetScreen(nextDirectory, true);
			}
		}
		else if (fileFlag) {
			openFile(nextDirectory);
		}

	}
	else if (ch == ':') {
		mode = 0;
	}

	else if (ch == KEY_LEFT) {
		string path = dirList.getPrevious();
		if (path.length() != 0) {
			resetScreen(path, false);
		}
	}

	else if (ch == KEY_RIGHT) {
		string path = dirList.getNext();
		if (path.length() != 0) {
			resetScreen(path, false);
		}
	}

	else if (ch == KEY_BACKSPACE) {
		resetScreen(currentDir + "/..", true);
	}

	else if (ch == 'h' or ch == 'H') {
		resetScreen(".", true);
	}

	else if (ch == 'k' or ch == 'K') {
		if (win.directionWindowLeft > 0)
			win.directionWindowLeft--;
		win.directionWindowRight = win.directionWindowLeft
		                           + win.windowSizeColumn - 4;

	}
	else if (ch == 'l' or ch == 'L') {
		win.directionWindowRight++;
		win.directionWindowLeft = win.directionWindowRight
		                          - win.windowSizeColumn + 4;
	}

}

void fileCopy(string nameFile, string dest_name) {
	ifstream  src(nameFile, std::ios::binary);
	ofstream  dst(dest_name, std::ios::binary);
	dst << src.rdbuf();
}


void fileMove(string currentPath, string nameFile, string pathDest) {
	string src = currentPath + "/" + nameFile;
	string dest = pathDest + "/" + nameFile;
	fileCopy(src, dest);
	remove(src.c_str());
}

void rename_file(string currentPath, string first, string second) {
	fileCopy(currentPath + "/" + first, currentPath + "/" + second);
	first = currentPath + "/" + first;
	remove(first.c_str());
}

bool search(string currentPath, string name) {
	vector<struct file_meta> fileList = readAllDirs(currentPath);
	struct stat sb;
	string nameFile;

	for (int i = 0; i < fileList.size(); i++) {
		nameFile = currentPath + "/" + fileList[i].name;
		if (isNotValid(nameFile, sb)) {
			continue;
		}

		if (isItFile(sb) and fileList[i].name == name) {
			return true;
		}
	}
	for (int i = 0; i < fileList.size(); i++) {
		if (isDefaultFile(fileList[i].name))
			continue;
		nameFile = currentPath + "/" + fileList[i].name;
		if (isNotValid(nameFile, sb))
			continue;
		if (isItFile(sb))
			continue;
		if (search(nameFile, name))
			return true;
	}
	return false;
}

void deletingDirectory(string currentPath) {
	vector<struct file_meta> fileList = readAllDirs(currentPath);
	string nameFile = "";
	struct stat sb;

	for (int i = 0; i < fileList.size(); i++) {
		nameFile = currentPath + "/" + fileList[i].name;
		if (isNotValid(nameFile, sb)) {
			continue;
		}

		if (isItFile(sb)) {
			remove(nameFile.c_str());
		}
	}
	for (int i = 0; i < fileList.size(); i++) {
		if (isDefaultFile(fileList[i].name))
			continue;
		nameFile = currentPath + "/" + fileList[i].name;
		if (isNotValid(nameFile, sb))
			continue;
		if (isItFile(sb))
			continue;
		deletingDirectory(nameFile);
		rmdir(nameFile.c_str());
	}
}



void copyingDirectory(string src_path, string pathDest) {
	vector<file_meta> fileList;
	fileList = readAllDirs(src_path);
	struct stat sb;
	string destFile, nameFile;

	for (int i = 0; i < fileList.size(); i++) {
		nameFile = src_path + "/" + fileList[i].name;
		if (isNotValid(nameFile, sb) or isDefaultFile(fileList[i].name)) {
			continue;
		}

		destFile = pathDest + "/" + fileList[i].name;

		if (isItFile(sb)) {
			fileCopy(nameFile, destFile);
		} else {
			mkdir(destFile.c_str(), 0777);
			copyingDirectory(nameFile, destFile);
		}
	}
}

void movingDirectory(string curr_path, string nameDir, string pathDest) {
	string destDirName = pathDest + "/" + nameDir;
	mkdir(destDirName.c_str(), 0777);
	string srcDirName = curr_path + "/" + nameDir;
	copyingDirectory(srcDirName, destDirName);
	deletingDirectory(srcDirName);
	rmdir(srcDirName.c_str());
}


string correctPath(string path, string currentPath) {
	if (path.length() == 0)
		return "";

	if (path[0] == '~')
		path[0] = '.';

	else
		path = currentPath + "/" + path;

	return path;
}



void copyHelper(vector<string>& inputPieces) {
	string pathDest = inputPieces.back();

	pathDest = correctPath(pathDest, currentDir);

	string nameFile, destFile;

	for (int i = 1; i < inputPieces.size() - 1; i++) {
		nameFile = currentDir + "/" + inputPieces[i];
		destFile = pathDest + "/" + inputPieces[i];
		struct stat sb;

		if (isNotValid(nameFile, sb)) {
			continue;
		}
		if (isItFile(sb)) {
			fileCopy(nameFile, destFile);
		}
		else {
			string nameDir = destFile;
			mkdir(nameDir.c_str(), 0777);
			copyingDirectory(nameFile, nameDir);
		}
	}
}

void moveHelper(vector<string>& inputPieces) {
	string pathDest = inputPieces.back();
	pathDest = correctPath(pathDest, currentDir);
	for (int i = 1; i < inputPieces.size() - 1; i++) {
		string nameFile = currentDir + "/" + inputPieces[i];
		struct stat sb;
		if (isNotValid(nameFile, sb))
			continue;
		if (isItFile(sb)) {
			fileMove(currentDir, inputPieces[i], pathDest);
		}
		else {
			movingDirectory(currentDir, inputPieces[i], pathDest);
		}
	}

}


vector<string> stringSplit(string input) {

	vector<string> arr;
	char delim[] = " ";
	char* inp = &input[0];
	char *token = strtok(inp, delim);

	while (token) {
		string s(token);
		arr.push_back(s);
		token = strtok(NULL, delim);
	}

	return arr;
}

void setCommandString(string& command, string s) {
	command = s;
}

void commandMode(string& command) {
	if (command.length() == 0) {
		command = "enter something...";
		return;
	}

	vector<string>inputPieces = stringSplit(command);

	if (inputPieces[0] == "copy") {
		if (inputPieces.size() < 3) {
			setCommandString(command, "log: Error");
		}

		copyHelper(inputPieces);
		setCommandString(command, "log: copied");

	}
	else if (inputPieces[0] == "move") {
		if (inputPieces.size() < 3) {
			setCommandString(command, "log: Error");
		}
		moveHelper(inputPieces);
		setCommandString(command, "log: moved");
	}


	else if (inputPieces[0] == "rename") {
		if (inputPieces.size() != 3) {
			setCommandString(command, "log: Error");
		}
		else {
			rename_file(currentDir, inputPieces[1], inputPieces[2]);
			command = "success";
		}
	}


	else if (inputPieces[0] == "create_file") {
		if (inputPieces.size() == 3) {
			string pathFile = correctPath(inputPieces[2], currentDir);
			string nameFile = pathFile + "/" + inputPieces[1];
			ofstream dst(nameFile, std::ios::binary);
			command = "success";

		}
		else {
			setCommandString(command, "log: Error");
		}
	}


	else if (inputPieces[0] == "create_dir") {
		if (inputPieces.size() == 3) {
			// command = "log: Err";
			string dir_path = inputPieces[2];
			dir_path = correctPath(dir_path, currentDir);
			string nameDir = dir_path + "/" + inputPieces[1];
			mkdir(nameDir.c_str(), 0777);
			command = "success";

		}
		else {
			setCommandString(command, "log: Error");
		}
	}


	else if (inputPieces[0] == "delete_file") {
		if (inputPieces.size() == 2) {
			string file_path = inputPieces[1];
			file_path = correctPath(file_path, currentDir);
			int rc = remove(file_path.c_str());
			command = "success";
		}
		else {
			setCommandString(command, "log: Error");
		}
	}


	else if (inputPieces[0] == "delete_dir") {
		if (inputPieces.size() == 2) {
			string dir_path = inputPieces[1];
			dir_path = correctPath(dir_path, currentDir);
			deletingDirectory(dir_path);
			rmdir(dir_path.c_str());
			command = "success";

		}
		else {
			setCommandString(command, "log: Error");
		}
	}


	else if (inputPieces[0] == "goto") {
		if (inputPieces.size() == 2) {
			string dir_path = inputPieces[1];
			dir_path = correctPath(dir_path, currentDir);
			// currentDir = dir_path;
			// files = readAllDirs(currentDir);
			cls;
			resetScreen(dir_path, true);
			displayAllDirs();
			setCommandString(command, "log: goto success");
		}
		else {
			setCommandString(command, "log: Error");
		}
	}


	else if (inputPieces[0] == "search") {
		if (inputPieces.size() != 2) {
			setCommandString(command, "log: Error");
			return;
		}

		if (search(currentDir, inputPieces[1]))
			setCommandString(command, inputPieces[1] + " found");
		else
			setCommandString(command, "not found");
	}
}

bool isWordInput(char ch) {
	return ' ' <= ch and ch <= '~';
}

bool unitlQuit(char ch) {
	return ch == 'q' or ch == 'Q';
}

int main()
{
	char ch;

	cls;
	int mode = 1;
	string command = "";
	char* buffer = getcwd( NULL, 0 );

	if (buffer == NULL) {
		perror("failed to get cwd");
		exit(1);
	}
	else {
		currentDir = buffer;
		free(buffer);
	}

	if (signal(SIGWINCH, sig_winch) == SIG_ERR)
		cerr << "signal error";
	sig_winch(1);

	dirList.insert(currentDir);

	enter_raw_mode();

	files = readAllDirs(currentDir);
	displayAllDirs();

	moveCursor(1, 0)
	arrow;
	display_footer();

	bool footerFlag = false;

	while (true)
	{
		if (mode == 1) {
			cls;
			displayAllDirs();
			arrow;
			display_footer();

			ch = cin.get();
			if (unitlQuit(ch))
				break;

			normalMode(ch, mode);

		}
		else {
			//command mode start
			displayFooterCommandMode(command);	//changing footer

			if (footerFlag) {
				command = "";
				footerFlag = false;
			}

			ch = cin.get();

			if (unitlQuit(ch) and command == "")
				break;

			if (ch == esc) {
				mode = 1;
				files = readAllDirs(currentDir);
			}

			else if (ch == KEY_BACKSPACE) {
				if (command.length() > 0)
					command.pop_back();
			}

			else if (isWordInput(ch)) {

				command.push_back(ch);
			}

			else if (ch == KEY_ENTER) {
				commandMode(command);
				footerFlag = true;
			}

		}
	}
	cls;
	exit_raw_mode;
	return 0;
}