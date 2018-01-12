//Pure virtual function for the basis of all GPUBuffers

#pragma once

#include "Compute/GPUDevice.h"
#include "glm/glm.hpp"

/*
*    Buffer for gpu/cpu communication and storage.
*    @tparam	T	Generic type parameter.
*/


template <class T>
class DeviceBuffer {

public:

	//Types

	typedef T                                     value_type;
	typedef uint		                          size_type;


	//Construction and Destruction 
	
	DeviceBuffer(const GPUDevice& _device);

	virtual ~DeviceBuffer();

	//Data Migration

	virtual void Move(const GPUDevice&) = 0;

	virtual void TransferToHost(std::vector<T>&) = 0;
	virtual void TransferToDevice(std::vector<T>&) = 0;


	//Element Access

	virtual T* Data() = 0;
	virtual const T* Data() const = 0;


	//Capacity

	virtual bool Empty() const noexcept = 0;

	virtual size_type Size() const noexcept = 0;

	virtual size_type MaxSize() const noexcept = 0;

	virtual size_type Capacity() const noexcept = 0;

	virtual void Resize(size_type) = 0;
	virtual void Resize(size_type, const T&) = 0;

	virtual void Reserve(size_type) = 0;

	virtual void Fit() = 0;

	//Misc
	
	GPUBackend GetAPI() const;
	
	
protected:

	uint size = 0;   // Number of objects
	uint capacity = 0;	// The capacity

	GPUDevice residentDevice;

	virtual void Reallocate() = 0;

};

template <class T>
DeviceBuffer<T>::DeviceBuffer(const GPUDevice& device):
	residentDevice(device){

}

template <class T>
DeviceBuffer<T>::~DeviceBuffer()
{
	
}

template <class T>
GPUBackend DeviceBuffer<T>::GetAPI() const
{
	return residentDevice.GetAPI();
}