#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include "CircularBuffer.h"
#include "RingList.h"



// Ўаблонна€ структура дл€ определени€ четности целого числа, основное отличие это то, что процесс вычислени€ происходит на этапе компил€ции
// ѕлюсы: уменьшение времени выполнени€
// ћинусы: увеличение времени компил€ции, структура принимает только константные выражени€ или литералы

template<int N>
struct isEven
{
	static const bool rezult = N % 2 == 0 ? true : false;
};


// ‘ункци€, котора€ сортирует массив по возрастанию
// «а первый проход находит max, также выполн€ет проверку на Ђотсортированностьї, ну и чтобы по максимуму получить пользы от этого прохода, начинает раскладывать значени€ по-разр€дам
// ≈сли массив отказалс€ неотсортированным, то происходит выбор лучшего способа сортировки (между быстрой и сортировкой разр€дами). 
// ≈сли массив небольшой и содержит числа больших разр€дов, то эффективнее будет использовать быструю сортировку
// ≈сли массив большой и содержит числа небольших разр€дов(например там много одинаковых значений), то эффективнее будет использовать сортировку разр€дами

void MySort(std::vector<size_t>& vec)
{
	if (vec.size() < 1)
		return;

	size_t max = vec[0];
	bool isSort = true;
	
	std::vector<std::vector<int>> ptr(20, std::vector<int>(vec.size()));
	std::vector<size_t> index_count = { 0,0,0,0,0,0,0,0,0,0 };
	std::vector<size_t> index_count2 = { 0,0,0,0,0,0,0,0,0,0 };

	for (size_t i = 1; i < vec.size(); ++i)
	{
		if (max < vec[i])
			max = vec[i];
	
		if (vec[i - 1] > vec[i])
		{
			isSort = false;
		}

		int a = vec[i - 1] % 10;
		ptr[a][index_count[a]++] = vec[i - 1];
	}

	if (isSort)
		return;

	int a = vec[vec.size() - 1] % 10;
	ptr[a][index_count[a]++] = vec[vec.size() - 1];
	
	rsize_t radix = std::floor(std::log10(max)) + 1;

	if (radix < std::log2(vec.size()))
	{
		int jump = 0;
				
		for (size_t a = 1; a < radix; ++a)
		{
			jump = jump == 0 ? 10 : 0;

			for (int i = 0; i < 10; ++i)
			{
				for(int j = 0; j < index_count[i]; ++j)
				{
					size_t val = ptr[i + (jump == 0 ? 10 : 0)][j];
					int b = static_cast<int>(val / std::pow(10, a)) % 10;
					ptr[b + jump][index_count2[b]++] = val;
				}
				index_count[i] = 0;
			}

			index_count2.swap(index_count);
		}

		int q = 0;
		for (int a = 0; a < 10; ++a)
		{
			for (int j = 0; j < index_count[a]; ++j)
			{
				vec[q++] = ptr[a + jump][j];
			}
		}
	}
	else
	{
		std::sort(vec.begin(), vec.end());
	}
	
};



int main()
{

	
	return 0;
}