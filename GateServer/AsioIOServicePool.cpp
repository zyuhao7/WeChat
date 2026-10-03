#include "AsioIOServicePool.h"
#include <iostream>
using namespace std;
AsioIOServicePool::AsioIOServicePool(std::size_t size) 
	:_ioServices(size),
	_works(size), 
	_nextIOService(0) {
	for (std::size_t i = 0; i < size; ++i) {
		_works[i] = std::unique_ptr<Work>(new Work(_ioServices[i]));
	}

	//iterate the io_services, create a thread for each and run its io_service
	for (std::size_t i = 0; i < _ioServices.size(); ++i) {
		_threads.emplace_back([this, i]() {
			_ioServices[i].run();
			});
	}
}

AsioIOServicePool::~AsioIOServicePool() {
	Stop();
	std::cout << "AsioIOServicePool destruct" << endl;
}

boost::asio::io_context& AsioIOServicePool::GetIOService() {
	auto& service = _ioServices[_nextIOService++];
	if (_nextIOService == _ioServices.size()) {
		_nextIOService = 0;
	}
	return service;
}

void AsioIOServicePool::Stop() {
	// work.reset alone does not make the io_context exit its run state
	// once the io_context has bound read/write handlers, it must still be stopped manually.
	for (auto& work : _works) {
		//stop the service first
		work->get_io_context().stop();
		work.reset();
	}
	for (auto& t : _threads) {
		t.join();
	}
}