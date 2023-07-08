// Investigate how std::vector::insert(...,InputIter begin, InputIter end) is implemented
#include <vector>

using namespace std;
int main()
{
    vector<int> vi(50,9);
    vector<int> insertMe(19,12);
    vi.insert(vi.begin()+6,insertMe.begin(),insertMe.end());
}