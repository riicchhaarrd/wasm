static int g = 456;
int test(int a)
{
	g += a;
	return 123 + a + g;
}
