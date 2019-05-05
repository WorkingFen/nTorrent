#include <iostream>
#include <vector>
using std::string;
using std::vector;
using std::istream;

extern string hashOnePiece(istream & stream, int pieceLength);
extern vector<string> hashPieces(string fileName, int pieceLength);
