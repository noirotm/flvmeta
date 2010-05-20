#include "MiniCppUnit.hxx"

int main()
{
	return TestFixtureFactory::theInstance().runTests() ? 0 : -1;
}	
