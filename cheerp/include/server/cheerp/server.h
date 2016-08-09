/****************************************************************
 *
 * Copyright (C) 2012-2014 Alessandro Pignotti <alessandro@leaningtech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 ***************************************************************/

#ifndef _CHEERP_SERVER_H
#define _CHEERP_SERVER_H

#include <utility>
#include <exception>
#include <iostream>
#include <string.h>
#include <cheerp/promise.h>
#include <cheerp/connection.h>

namespace cheerp
{

class DeserializationException//: public std::exception
{
private:
	const char* message;
public:
	DeserializationException(const char* m):message(m)
	{
	}
	const char* what() const throw()
	{
		return message;
	}
};

template<typename T>
struct serializeImpl
{
	static void run(SerializationInterface* outData, const T& data)
	{
		data.serialize(outData);
	}
};

template<>
struct serializeImpl<bool>
{
	static void run(SerializationInterface* outData, bool data)
	{
		if(data)
			outData->write("1",1);
		else
			outData->write("0",1);
	}
};

inline void serializeInt(SerializationInterface* outData, int data)
{
	int reqBuf = snprintf(NULL,0,"%i",data);
	if(reqBuf < 0)
		return;
	char buf[reqBuf+1];
	sprintf(buf,"%i",data);
	outData->write(buf, reqBuf);
}

inline void serializeUnsignedInt(SerializationInterface* outData, unsigned int data)
{
	int reqBuf = snprintf(NULL,0,"%u",data);
	if(reqBuf < 0)
		return;
	char buf[reqBuf+1];
	sprintf(buf,"%u",data);
	outData->write(buf, reqBuf);
}

inline void serializeDouble(SerializationInterface* outData, double data)
{
	int reqBuf = snprintf(NULL,0,"%g",data);
	if(reqBuf < 0)
		return;
	char buf[reqBuf+1];
	sprintf(buf,"%g",data);
	outData->write(buf, reqBuf);
}

template<>
struct serializeImpl<char>
{
	static void run(SerializationInterface* outData, char data)
	{
		serializeInt(outData, data);
	}
};

template<>
struct serializeImpl<unsigned char>
{
	static void run(SerializationInterface* outData, unsigned char data)
	{
		serializeUnsignedInt(outData, data);
	}
};

template<>
struct serializeImpl<int>
{
	static void run(SerializationInterface* outData, int data)
	{
		serializeInt(outData, data);
	}
};

template<>
struct serializeImpl<unsigned int>
{
	static void run(SerializationInterface* outData, unsigned int data)
	{
		serializeUnsignedInt(outData, data);
	}
};

template<>
struct serializeImpl<float>
{
	static void run(SerializationInterface* outData, float data)
	{
		serializeDouble(outData, data);
	}
};

template<>
struct serializeImpl<std::string>
{
	static void run(SerializationInterface* outData, const std::string& data)
	{
		outData->write(data.data(), data.size());
	}
};

template<class InputIterator>
inline void serializeRange(cheerp::SerializationInterface* outData, InputIterator begin, const InputIterator end)
{
	//Serialize as an array
	outData->write("[",1);
	bool first=true;
	for(;begin!=end;++begin)
	{
		if(!first)
			outData->write(",",1);
		serializeImpl<
			typename std::remove_const<typename std::remove_reference<decltype(*begin)>::type>::type
			>::run(outData, *begin);
		first=false;
	}
	outData->write("]",1);
}

template<class T>
struct serializeImpl<std::vector<T>>
{
	static void run(cheerp::SerializationInterface* outData, const std::vector<T>& data)
	{
		serializeRange(outData, data.begin(), data.end());
	}
};

template<typename T>
inline void serialize(cheerp::SerializationInterface* outData, const T& data)
{
	return serializeImpl<T>::run(outData, data);
}

template<class T>
T deserialize(const char*& data)
{
	return T::deserialize(data);
}

template<>
unsigned char deserialize(const char*& data);

template<>
unsigned int deserialize(const char*& data);

template<>
int deserialize(const char*& data);

template<>
float deserialize(const char*& data);

template<>
const std::string deserialize(const char*& data);

template<class OutputIterator>
inline void deserializeArrayInPlace(OutputIterator begin, const OutputIterator end, const char*& data)
{
	//First char is '['
	data++;
	for(;begin!=end;++begin)
	{
		*begin=deserialize<
			typename std::remove_reference<decltype(*begin)>::type
		>(data);
		//A comma follows
		data++;
	}
}


//This is used for the no arguments case
template<typename Signature, Signature Func, typename Ret, typename ...Args>
struct argumentDeserializer
{
	template<typename ...FuncArgs>
	static Ret execute(const char* data, FuncArgs&&... funcArgs)
	{
		//Arguments are passed as array, skip the first parenthesis
		if(data[0]!=']')
			throw DeserializationException("Malformed arguments array");
		//Finally call the method
		return Func(std::forward<FuncArgs>(funcArgs)...);
	}
};

//Base version for no arguments
template<typename Signature, Signature Func, typename Ret, typename Deserialize, typename ...Args>
struct argumentDeserializer<Signature, Func, Ret, Deserialize, Args...>
{
	template<typename ...FuncArgs>
	static Ret execute(const char* data, FuncArgs&&... funcArgs)
	{
		const Deserialize& d=deserialize<typename std::remove_reference<Deserialize>::type>(data);
		//Expect a comma if we expect more args
		if(sizeof...(Args)>0)
		{
			if(data[0]!=',')
				throw DeserializationException("Malformed arguments array");
			else
				data++;
		}
		//Pass down the updated data, the previous args and the new arg
		return argumentDeserializer<Signature,Func,Ret,Args...>::
			execute(data, std::forward<FuncArgs>(funcArgs)..., d);
	}
};

template<class R>
struct voidUtils
{
	static void addCallback(Promise<R>* p)
	{
		Connection* c=connection;
		p->then([c] (const R& r) mutable {
			cheerp::serializeImpl<R>::run(c, r);
			c->flush();
			c->send();
			});
		p->complete();
	}
};

template<>
struct voidUtils<void>
{
	static void addCallback(Promise<void>* p)
	{
		Connection* c(connection);
		p->then([c]() mutable {
			c->flush();
			c->send();
			});
		p->complete();
	}
};

template<typename Signature, Signature Func, typename Ret, typename ...Args>
struct returnSerializer
{
	static PromiseBase* serialize(SerializationInterface* outData, const char* inData)
	{
		const Ret& r=argumentDeserializer<Signature,Func,Ret,Args...>::execute(inData);
		cheerp::serializeImpl<Ret>::run(outData, r);
		return NULL;
	}
};

template<typename Signature, Signature Func, typename ...Args>
struct returnSerializer<Signature,Func,void,Args...>
{
	static PromiseBase* serialize(SerializationInterface* outData, const char* inData)
	{
		argumentDeserializer<Signature,Func,void,Args...>::execute(inData);
		return NULL;
	}
};

template<typename Signature, Signature Func, typename Ret, typename ...Args>
struct returnSerializer<Signature,Func,Promise<Ret>*,Args...>
{
	static PromiseBase* serialize(SerializationInterface* outData, const char* inData)
	{
		argumentDeserializer<Signature,Func,Promise<Ret>*,Args...> serializer;
		Promise<Ret>* ret=serializer.execute(inData);
		voidUtils<Ret>::addCallback(ret);
		// Save connection by copy in the lambda, this seems to require a temporary
		return ret;
	}
};

}

/*
 * The output buffer is assumed to be 1024 bytes in size
 * It the method returns a promise it is forwarded to the caller
 */
template<typename Signature, Signature Func, typename Ret, typename ...Args>
cheerp::PromiseBase* serverSkel(cheerp::SerializationInterface* outData, const char* inData)
{
	try
	{
		//Arguments are passed as array, skip the first parenthesis
		if(inData[0]!='[')
			throw cheerp::DeserializationException("Missing [ at the start of parameters");
		return cheerp::returnSerializer<Signature,Func,Ret,Args...>::serialize(outData,inData+1);
	}
	catch(cheerp::DeserializationException& e)
	{
		std::cerr << e.what() << std::endl;
	}
	return NULL;
}
#endif
