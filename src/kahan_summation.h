#pragma once
// via http://ideone.com/iuPPP and https://stackoverflow.com/questions/10330002/sum-of-small-double-numbers-c
// modified by Dustin Kaiser August 2017
template <typename T>
struct KahanAccumulation
{
	T sum = T(0);
	T correction = T(0);
};

template <typename T>
KahanAccumulation<T> KahanSum(KahanAccumulation<T> const& accumulation, T value)
{
	KahanAccumulation<T> result;
	T y = value - accumulation.correction;
	T t = accumulation.sum + y;
	result.correction = (t - accumulation.sum) - y;
	result.sum = t;
	return result;
}