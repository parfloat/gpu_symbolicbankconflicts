#ifndef PROBABILITIES_H_
#define PROBABILITIES_H_

#include <algorithm>
#include <cmath>
#include <vector>
#include <iostream>

//Interquartile range
template<typename T>
static inline double lerp(T v0, T v1, T t)
{
    return (1 - t)*v0 + t*v1;
}

template<typename T>
static inline std::vector<T> quantile(const std::vector<T>& inData, const std::vector<T>& probs)
{
    if (inData.empty())
    {
        return std::vector<T>();
    }

    if (1 == inData.size())
    {
        return std::vector<T>(1, inData[0]);
    }

    std::vector<T> data = inData;
    std::sort(data.begin(), data.end());
    std::vector<T> quantiles;

    for (size_t i = 0; i < probs.size(); ++i)
    {
        T poi = lerp<T>(-0.5, data.size() - 0.5, probs[i]);

        size_t left = std::max(int64_t(std::floor(poi)), int64_t(0));
        size_t right = std::min(int64_t(std::ceil(poi)), int64_t(data.size() - 1));

        T datLeft = data.at(left);
        T datRight = data.at(right);

        T quantile = lerp<T>(datLeft, datRight, poi - left);

        quantiles.push_back(quantile);
    }

    return quantiles;
}

template<typename T>
static inline std::vector<T> removeOutliers(const std::vector<T>& inData, const std::vector<T>& quantiles, const float distr = 1.5f){
    std::vector<T> outData;
    // std::cout << quantiles[0] << " " << quantiles[1] << " " << quantiles[2] << "\n";
    T left = quantiles[0] - distr * (quantiles[2] - quantiles[0]);
    // std::cout << "left = " << left << "\n";
    T right = quantiles[2] + distr * (quantiles[2] - quantiles[0]);
    // std::cout << "right = " << right << "\n";
    for (int i = 0; i < inData.size(); ++i){
      if ((left <= inData[i]) && (inData[i] <= right)){
        outData.push_back(inData[i]);
      }
    }

    return outData;
}

template<typename T>
T getAverage(const std::vector<T>& inData){
  T avg = 0;
  for (int i = 0; i < inData.size(); ++i){
      avg += inData[i];
  }

  return (avg / inData.size());
}

template<typename T>
T getAverageIRQ(const std::vector<T>& inData, const std::vector<T>& probs={0.25, 0.5, 0.75}, const float distr = 1.5f){
  auto quartiles = quantile<T>(inData, { 0.25, 0.5, 0.75 });
	auto outData = removeOutliers<T>(inData, quartiles);
	T avg = getAverage<T>(outData);

  return avg;
}


#endif
