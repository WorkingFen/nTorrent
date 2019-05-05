#include "headers/hash.h"
using namespace std;




int main (){
    vector<string> hashes = hashPieces("example.txt", 2);

    for(auto it=hashes.begin(); it != hashes.end(); ++it){
        cout<<"sha256:"<<*it<<endl;
    }
    return 0;
}