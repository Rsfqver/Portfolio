#include<iostream>
#include <fstream>
#include <vector>
#include <string>
#include <ctime>
#include <cstdlib>
#include <set>


using namespace std;
//클래스의 멤버 변수들 
class HangMan
{
    ifstream fin; // 파일 열기 위한 변수 지정 
    vector<string> v;
    string originalWord; // 원본 단어
public:
    HangMan();
    void fileRead(); // 파일 읽는 구성부 
    string maketext(); // 텍스트 생성 // 문제 구성부 
    string Answer(); // 게임 답변 
    void run(); // 실행파일 exe
    ~HangMan(); // 소멸자 
};

HangMan::HangMan()
{
    const char* file = "words.txt";
    fin.open(file);
    if (!fin) // 파일 열기 실패 조건 수정
    {
        cout << file << " 파일 열기에 실패" << endl;
        exit(1);
    }
}

void HangMan::fileRead()
{
    string line;
    while (getline(fin, line))
    {
        if (!line.empty()) // 빈 줄 무시
            v.push_back(line);
    }
}

string HangMan::maketext()
{
    srand((unsigned)time(NULL)); // 시드 초기화
    int n = rand() % v.size(); // 벡터 크기로 수정
    originalWord = v.at(n);

    // 글자 수가 2개 이상인지 확인
    if (originalWord.size() < 2)
    {
        cout << "단어가 너무 짧습니다." << endl;
        exit(1);
    }

    // 2개의 랜덤 인덱스를 선택 (중복 허용)
    set<char> uniqueChars; // 중복 방지를 위한 set
    while (uniqueChars.size() < 2)
    {
        int index = rand() % originalWord.size();
        uniqueChars.insert(originalWord[index]); // 중복된 글자는 자동으로 무시됨
    }

    // 보이는 글자의 인덱스와 글자를 저장
    string displayWord(originalWord.size(), '-'); // 초기화
    for (char c : uniqueChars) {
        for (size_t j = 0; j < originalWord.size(); j++) {
            if (originalWord[j] == c) {
                displayWord[j] = c; // 초기 상태에서 보이게 할 글자를 설정
            }
        }
    }

    return displayWord;
}

string HangMan::Answer() // 게임 답변
{
    string displayWord = maketext(); // 처음에 화면에 보일 단어 생성
    int fcount = 0; // 틀린 개수

    while (true)
    {
        cout << displayWord << endl;
        cout << ">> ";
        char user;
        cin >> user;

        bool found = false; // 글자가 발견되었는지 여부
        for (size_t i = 0; i < originalWord.size(); i++)
        {
            if (user == originalWord[i])
            {
                found = true;
                displayWord[i] = user; // 맞는 글자일 경우 displayWord 업데이트
            }
        }

        if (found) {
           // cout << "맞았습니다!" << endl;
            if (displayWord == originalWord) {
               // "정답입니다" cout << displayWord << endl;
                return originalWord;
            }
        }
        else {
            fcount++;
            //cout << "틀렸습니다. 남은 기회: " << 5 - fcount << endl;
        }

        if (fcount == 5) {
            cout << "5번 실패로 게임이 종료됩니다. 정답은: " << originalWord << endl;
            return "게임 종료";
        }
    }
}

HangMan::~HangMan()
{
    fin.close(); // 소멸자에서 파일 닫기
}

void HangMan::run()
{
    cout << "---------------------" << endl;
    cout << "The Hangman game starts now" << endl;
    cout << "---------------------" << endl;
    fileRead();
    while (true)
    {
        string result = Answer();
        cout << result << endl;

        char go;
        while (true)
        {
            cout << "Next(y/n)? ";
            cin >> go;
            if (go == 'y') break;
            else if (go == 'n') exit(0);
            else
            {
                cout << "잘못 입력하셨습니다. 다시 입력하세요." << endl;
            }
        }
    }
}

int main()
{
    HangMan a;
    a.run();
}