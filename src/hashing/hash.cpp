#include <fstream>
#include <cmath>
#include "headers/sha256.h"
#include "headers/hash.h"

using std::string;
using std::cout;
using std::endl;
using std::ifstream;
using std::istream;
using std::ios;
using std::vector;
using std::uint32_t;

string hashOnePiece(istream & stream, int pieceLength) {
	string bytes(pieceLength, ' ');

	stream.read(&bytes[0], pieceLength);

	string hash = sha256(bytes);

	//cout<<"sha256:"<<hash<<endl;

	return hash;
}

vector<string> hashPieces(string fileName, int pieceLength) {

    /*
    *  Funkcja oblicza sha256 dla fragmentów danego pliku.
    *  fileName: ścieżka do pliku
    *  pieceLength: rozmiar fragmentu w bajtach
    *  return: wektor haszy
    */

	ifstream is(fileName.c_str(), ios::in | ios::binary | ios::ate);
	int fileLength = is.tellg();
	is.seekg(0, is.beg);

	int piecesCount = int(ceil(double(fileLength) / double(pieceLength)));

	vector<string> hashes;

	for (int i = 0; i < piecesCount - 1; ++i) {

		hashes.push_back(hashOnePiece(is, pieceLength));
	}
	// Ostatni fragment może być krótszy niż pieceLength
	int lastPieceLength = fileLength - (piecesCount - 1) * pieceLength;
	hashes.push_back(hashOnePiece(is, lastPieceLength));

	return hashes;
}
