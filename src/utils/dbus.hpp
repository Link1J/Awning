#pragma once

#include <dbus/dbus.h>
#include <string>
#include <iostream>

namespace DBus
{
	class Error
	{
		DBusError error;

	public:
		Error()
		{
			dbus_error_init(&error);
		}

		DBusError* GetError()
		{
			return &error;
		}

		const char* GetMessage()
		{
			return error.message;
		}

		bool IsSet()
		{
			return dbus_error_is_set(&error);
		}

		Error& operator=(Error&& other)
		{
			dbus_move_error(other.GetError(), GetError());
			return *this;
		}

		void Set(std::string name, std::string message)
		{
			char* namep = (char*)malloc(name.size() + 1);
			char* messagep = (char*)malloc(name.size() + 1);

			memcpy(namep, name.c_str(), name.size() + 1);
			memcpy(messagep, message.c_str(), message.size() + 1);

			dbus_set_error_const(&error, namep, messagep);
		}

		const char* GetName()
		{
			return error.name;
		}
	};

	class ObjectPath {};
	class Variant    {};
	class Array      {};

	template<typename T> int Type();

	template<> int Type<double     >() { return DBUS_TYPE_DOUBLE     ; }
	template<> int Type<const char*>() { return DBUS_TYPE_STRING     ; }
	template<> int Type<ObjectPath >() { return DBUS_TYPE_OBJECT_PATH; }
	template<> int Type<int32_t    >() { return DBUS_TYPE_INT32      ; }
	template<> int Type<uint32_t   >() { return DBUS_TYPE_UINT32     ; }
	template<> int Type<int16_t    >() { return DBUS_TYPE_INT16      ; }
	template<> int Type<uint16_t   >() { return DBUS_TYPE_UINT16     ; }
	template<> int Type<int64_t    >() { return DBUS_TYPE_INT64      ; }
	template<> int Type<uint64_t   >() { return DBUS_TYPE_UINT64     ; }
	template<> int Type<uint8_t    >() { return DBUS_TYPE_BYTE       ; }
	template<> int Type<Array      >() { return DBUS_TYPE_ARRAY      ; }
	template<> int Type<Variant    >() { return DBUS_TYPE_VARIANT    ; }

	class Message
	{
		DBusMessage* message = nullptr;

	public:
		class Iter
		{
			friend Message;
			DBusMessageIter iter;

		protected:			
			Iter(Message& message)
			{
				dbus_message_iter_init(message.GetMessage(), &iter);
			}
			
			Iter(DBusMessageIter iter)
			{
				this->iter = iter;
			}

		public:
			Iter() = default;

			int Type()
			{
				return dbus_message_iter_get_arg_type(&iter);
			}

			Iter Recurse()
			{
				DBusMessageIter sub;
				dbus_message_iter_recurse(&iter, &sub);
				return Iter(sub);
			}

			template<typename Return>
			Return Get()
			{
				Return returnValue;
				dbus_message_iter_get_basic(&iter, &returnValue);
				return returnValue;
			}

			template<typename Return>
			Return Get_Array(int& numberOfItems)
			{
				Return type;
				dbus_message_iter_get_fixed_array(&iter, &type, &numberOfItems);
				return type;
			}

			bool Next()
			{
				return dbus_message_iter_next(&iter);
			}
		};

		Message() = default;

		Message(std::string busName, std::string path, std::string interface, std::string method)
		{
			message = dbus_message_new_method_call(busName.c_str(), path.c_str(), interface.c_str(), method.c_str());
		}

		Message(DBusMessage* baseMessage)
		{
			message = baseMessage;
		}

		void Unref()
		{
			if (message)
				dbus_message_unref(message);
		}

		template<typename... Args>
		void Add(int first, Args... args)
		{
			dbus_message_append_args(message, first, args..., DBUS_TYPE_INVALID);
		}

		DBusMessage* GetMessage()
		{
			return message;
		}

		template<typename Return, typename RealType = Return>
		Return Get(Error& error)
		{
			Return type;
			dbus_message_get_args(message, error.GetError(), Type<RealType>(), &type, DBUS_TYPE_INVALID);
			return type;
		}

		Iter Get()
		{
			return Iter(*this);
		}
	};

	class Connection
	{
		DBusConnection* connection = nullptr;

	public: 
		Connection() = default;

		Connection(DBusBusType type, Error& error)
		{
			connection = dbus_bus_get(type, error.GetError());
		}

		DBusConnection* GetConnection()
		{
			return connection;
		}

		Message Send_Reply_Block(Message& message, int timeout, Error& error)
		{
			return Message(dbus_connection_send_with_reply_and_block(connection, message.GetMessage(), timeout, error.GetError()));
		}
	};

	template<typename Return, typename RealType = Return, bool array = false> 
	Return GetProperty(Connection& connection, std::string bus_name, std::string path, std::string iface, std::string propname)
	{
		Error error;

		Message queryMessage(bus_name, path, "org.freedesktop.DBus.Properties", "Get");
		queryMessage.Add(DBUS_TYPE_STRING, &iface, DBUS_TYPE_STRING, &propname);

		Message replyMessage = connection.Send_Reply_Block(queryMessage, 1000, error);
		queryMessage.Unref();

        if (error.IsSet())
		{
			std::cout << error.GetName() << " " << error.GetMessage() << "\n";
			return 0;
		}

		Message::Iter variant = replyMessage.Get();

		if (variant.Type() != Type<Variant>())
			return 0;

		Message::Iter data = variant.Recurse();

		Return result;

		if constexpr (array)
		{
			if (data.Type() != Type<Array>())
				return 0;

			Message::Iter inArray = data.Recurse();

			if (inArray.Type() != Type<RealType>())
				return 0;

			if (dbus_type_is_fixed(Type<RealType>()))
			{
				int numberOfItems = 0;
				Return temp = inArray.Get_Array<Return>(numberOfItems);

				result = (Return)malloc(numberOfItems * sizeof(Return));
				memcpy(result, temp, numberOfItems * sizeof(Return));
			}
			else
			{
				int numberOfItems = 0;
				inArray.Get_Array<Return>(numberOfItems);

				result = (Return)malloc(numberOfItems * sizeof(Return));
				
				int a = 0;
				do 
				{
					const char* temp = inArray.Get<const char*>();
					int tempLen = strlen(temp);
					result[a] = (const char*)malloc(tempLen);
					memcpy((void*)result[a], (void*)temp, tempLen);
					a++;
				}
				while (inArray.Next());
			}
		}
		else
		{
			if (data.Type() != Type<RealType>())
				return 0;

			result = data.Get<Return>();
		}

		replyMessage.Unref();

		return result;
	}

	template<typename Return, typename RealType = Return> 
	Return CallMethod(Connection& connection, std::string bus_name, std::string path, std::string iface, std::string method)
	{
		DBus::Error error;
		DBus::Message queryMessage(bus_name, path, iface, method);
	
		DBus::Message replyMessage = connection.Send_Reply_Block(queryMessage, 1000, error);
		queryMessage.Unref();
        if (error.IsSet())
		{
			std::cout << error.GetName() << " " << error.GetMessage() << "\n";
			return 0;
		}

		Return returnData = replyMessage.Get<Return, RealType>(error);
		replyMessage.Unref();
        if (error.IsSet())
		{
			std::cout << error.GetName() << " " << error.GetMessage() << "\n";
			return 0;
		}

		return returnData;
	}
}