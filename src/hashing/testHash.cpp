#include "headers/hash.h"
using namespace std;




int main (){
    vector<string> hashes = hashPieces("example.txt", 10);

    for(string s : hashes)
    {
        cout << "sha256:" << s <<endl;
    }

    cout << hashes.size() << endl;

    return 0;
}