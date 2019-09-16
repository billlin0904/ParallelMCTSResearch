// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <array>
#include <stdexcept>
#include <type_traits>
#include <cassert>

namespace xiangqi {

template <typename T, size_t N>
class SmallVector {
public:
	using value_type = T;
	using iterator = T*;
	using const_iterator = const T*;

	SmallVector() noexcept
		: length_(0) {
	}

	template <typename Iterator>
	SmallVector(Iterator begin, Iterator end) 
		: SmallVector() {
		Append(begin, end);
	}

	SmallVector(const std::initializer_list<T>& list) 
		: SmallVector(std::begin(list), std::end(list)) {
	}

	SmallVector(const SmallVector& other)
		: SmallVector() {
		AppendCopy(std::begin(other), std::end(other));
	}

	SmallVector& operator=(const SmallVector& other) {
		if (this != &other) {
			clear();
			AppendCopy(std::begin(other), std::end(other));
		}
		return *this;
	}

	SmallVector(SmallVector&& other) 
		: SmallVector() {
		AppendMove(std::begin(other), std::end(other));
		other.length_ = 0;
	}

	SmallVector& operator=(SmallVector&& other) {
		if (this != &other) {
			clear();
			AppendMove(std::begin(other), std::end(other));
			other.length_ = 0;
		}		
		return *this;
	}

	~SmallVector() {
		clear();
	}

	constexpr size_t size() const noexcept {
		return length_;
	}

	constexpr bool empty() const noexcept {
		return size() == 0;
	}

	void clear() {
		Destory<T>(begin(), end());
	}

	void push_back(const T& value) {
		InternalPushBack(value);
	}

	void push_back(T&& value) {
		InternalPushBack(std::move(value));
	}

	template <typename... Args>
	void emplace_back(Args&& ... args) {
		InternalPushBack(std::forward<Args>(args)...);
	}

	value_type& operator[](size_t index) noexcept {
		return buffer_[index];
	}

	const value_type& operator[](size_t index) const noexcept {
		return buffer_[index];
	}

	iterator begin() noexcept {
		return buffer_.data();
	}

	iterator end() noexcept {
		return buffer_.data() + length_;
	}

	const_iterator begin() const noexcept {
		return buffer_.data();
	}

	const_iterator end() const noexcept {
		return buffer_.data() + length_;
	}

	const_iterator cbegin() const noexcept {
		return begin();
	}

	const_iterator cend() const noexcept {
		return end();
	}

	iterator erase(iterator itr) {
		iterator N = itr;
		std::copy(itr + 1, this->end(), itr);
		pop_back();
		return N;
	}

	iterator erase(iterator S, iterator E) {
		iterator N = S;
		iterator I = std::copy(E, this->end(), S);
		Destory(I, this->end());
		return N;
	}

	void pop_back() {
		assert(length_ > 0);
		--length_;
		Destory<value_type>(buffer_[length_]);
	}

private:
	template <typename U>
	void Destory(U& t, typename std::enable_if<std::is_trivially_destructible<U>::value>::type* = 0) {
		t.~T();
	}

	template
	<
		typename U,
		typename std::enable_if<!std::is_trivially_destructible<U>{} && (std::is_class<U>{} || std::is_union<U>{}), int>::type = 0
	>
	void Destory(U&) {
	}

	template <typename U, typename Iterator>
	void Destory(Iterator begin, Iterator end) {
		for (auto itr = begin; itr != end; ++itr) {
			Destory<T>(*itr);
			--length_;
		}
	}

	template <typename Iterator>
	void Append(Iterator begin, Iterator end) {
		for (auto itr = begin; itr != end; ++itr) {
			push_back(*itr);
		}
	}

	template <typename Iterator>
	void AppendMove(Iterator begin, Iterator end) {
		for (auto itr = begin; itr != end; ++itr) {
			push_back(std::move(*itr));
		}
	}

	template <typename Iterator>
	void AppendCopy(Iterator begin, Iterator end) {
		for (auto itr = begin; itr != end; ++itr) {
			push_back(*itr);
		}
	}

	template <typename... Args>
	void InternalPushBack(Args&& ... args) {
		if (length_ >= N) {
#ifdef _DEBUG
			assert(0 && "Out of range");
#else
			throw std::out_of_range("Out of range");
#endif
		}
		void* placement = buffer_.data() + length_;
		new(placement) T(std::forward<Args>(args)...);
		++length_;
	}

	void CheckLength(size_t index) const {
		if (index >= length_) {
#ifdef _DEBUG
			assert(0 && "Out of range");
#else
			throw std::out_of_range("Out of range");
#endif
		}
	}

	size_t length_;
	std::array<T, N> buffer_;
};


}
