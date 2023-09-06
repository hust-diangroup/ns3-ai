/*
 * Copyright (c) 2018 Piotr Gawlowicz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Piotr Gawlowicz <gawlowicz.p@gmail.com>
 * Modify: Muyuan Shen <muyuan_shen@hust.edu.cn>
 *
 */

#ifndef OPENGYM_CONTAINER_H
#define OPENGYM_CONTAINER_H

#include "messages.pb.h"

#include <ns3/object.h>
#include <ns3/type-name.h>

namespace ns3
{

class OpenGymDataContainer : public Object
{
  public:
    OpenGymDataContainer();
    ~OpenGymDataContainer() override;

    static TypeId GetTypeId();

    virtual ns3_ai_gym::DataContainer GetDataContainerPbMsg() = 0;
    static Ptr<OpenGymDataContainer> CreateFromDataContainerPbMsg(
        ns3_ai_gym::DataContainer& dataContainer);

    virtual void Print(std::ostream& where) const = 0;

    friend std::ostream& operator<<(std::ostream& os, const Ptr<OpenGymDataContainer> container)
    {
        container->Print(os);
        return os;
    }

  protected:
    // Inherited
    void DoInitialize() override;
    void DoDispose() override;
};

class OpenGymDiscreteContainer : public OpenGymDataContainer
{
  public:
    OpenGymDiscreteContainer();
    OpenGymDiscreteContainer(uint32_t n);
    ~OpenGymDiscreteContainer() override;

    static TypeId GetTypeId();

    ns3_ai_gym::DataContainer GetDataContainerPbMsg() override;

    void Print(std::ostream& where) const override;

    friend std::ostream& operator<<(std::ostream& os, const Ptr<OpenGymDiscreteContainer> container)
    {
        container->Print(os);
        return os;
    }

    bool SetValue(uint32_t value);
    uint32_t GetValue() const;

  protected:
    // Inherited
    void DoInitialize() override;
    void DoDispose() override;

    uint32_t m_n;
    uint32_t m_value;
};

template <typename T = float>
class OpenGymBoxContainer : public OpenGymDataContainer
{
  public:
    OpenGymBoxContainer();
    OpenGymBoxContainer(std::vector<uint32_t> shape);
    ~OpenGymBoxContainer() override;

    static TypeId GetTypeId();

    ns3_ai_gym::DataContainer GetDataContainerPbMsg() override;

    void Print(std::ostream& where) const override;

    friend std::ostream& operator<<(std::ostream& os, const Ptr<OpenGymBoxContainer> container)
    {
        container->Print(os);
        return os;
    }

    bool AddValue(T value);
    T GetValue(uint32_t idx);

    bool SetData(std::vector<T> data);
    std::vector<T> GetData();

    std::vector<uint32_t> GetShape();

  protected:
    // Inherited
    void DoInitialize() override;
    void DoDispose() override;

  private:
    void SetDtype();
    std::vector<uint32_t> m_shape;
    ns3_ai_gym::Dtype m_dtype;
    std::vector<T> m_data;
};

template <typename T>
TypeId
OpenGymBoxContainer<T>::GetTypeId()
{
    std::string name = TypeNameGet<T>();
    static TypeId tid = TypeId("ns3::OpenGymBoxContainer<" + name + ">")
                            .SetParent<Object>()
                            .SetGroupName("OpenGym")
                            .template AddConstructor<OpenGymBoxContainer<T>>();
    return tid;
}

template <typename T>
OpenGymBoxContainer<T>::OpenGymBoxContainer()
{
    SetDtype();
}

template <typename T>
OpenGymBoxContainer<T>::OpenGymBoxContainer(std::vector<uint32_t> shape)
    : m_shape(shape)
{
    SetDtype();
}

template <typename T>
OpenGymBoxContainer<T>::~OpenGymBoxContainer()
{
}

template <typename T>
void
OpenGymBoxContainer<T>::SetDtype()
{
    std::string name = TypeNameGet<T>();
    if (name == "int8_t" || name == "int16_t" || name == "int32_t" || name == "int64_t")
    {
        m_dtype = ns3_ai_gym::INT;
    }
    else if (name == "uint8_t" || name == "uint16_t" || name == "uint32_t" || name == "uint64_t")
    {
        m_dtype = ns3_ai_gym::UINT;
    }
    else if (name == "float")
    {
        m_dtype = ns3_ai_gym::FLOAT;
    }
    else if (name == "double")
    {
        m_dtype = ns3_ai_gym::DOUBLE;
    }
    else
    {
        m_dtype = ns3_ai_gym::FLOAT;
    }
}

template <typename T>
void
OpenGymBoxContainer<T>::DoDispose()
{
}

template <typename T>
void
OpenGymBoxContainer<T>::DoInitialize()
{
}

template <typename T>
ns3_ai_gym::DataContainer
OpenGymBoxContainer<T>::GetDataContainerPbMsg()
{
    ns3_ai_gym::DataContainer dataContainerPbMsg;
    ns3_ai_gym::BoxDataContainer boxContainerPbMsg;

    std::vector<uint32_t> shape = GetShape();
    *boxContainerPbMsg.mutable_shape() = {shape.begin(), shape.end()};

    boxContainerPbMsg.set_dtype(m_dtype);
    std::vector<T> data = GetData();

    if (m_dtype == ns3_ai_gym::INT)
    {
        *boxContainerPbMsg.mutable_intdata() = {data.begin(), data.end()};
    }
    else if (m_dtype == ns3_ai_gym::UINT)
    {
        *boxContainerPbMsg.mutable_uintdata() = {data.begin(), data.end()};
    }
    else if (m_dtype == ns3_ai_gym::FLOAT)
    {
        *boxContainerPbMsg.mutable_floatdata() = {data.begin(), data.end()};
    }
    else if (m_dtype == ns3_ai_gym::DOUBLE)
    {
        *boxContainerPbMsg.mutable_doubledata() = {data.begin(), data.end()};
    }
    else
    {
        *boxContainerPbMsg.mutable_floatdata() = {data.begin(), data.end()};
    }

    dataContainerPbMsg.set_type(ns3_ai_gym::Box);
    dataContainerPbMsg.mutable_data()->PackFrom(boxContainerPbMsg);
    return dataContainerPbMsg;
}

template <typename T>
bool
OpenGymBoxContainer<T>::AddValue(T value)
{
    m_data.push_back(value);
    return true;
}

template <typename T>
T
OpenGymBoxContainer<T>::GetValue(uint32_t idx)
{
    T data = 0;
    if (idx < m_data.size())
    {
        data = m_data.at(idx);
    }
    return data;
}

template <typename T>
bool
OpenGymBoxContainer<T>::SetData(std::vector<T> data)
{
    m_data = data;
    return true;
}

template <typename T>
std::vector<uint32_t>
OpenGymBoxContainer<T>::GetShape()
{
    return m_shape;
}

template <typename T>
std::vector<T>
OpenGymBoxContainer<T>::GetData()
{
    return m_data;
}

template <typename T>
void
OpenGymBoxContainer<T>::Print(std::ostream& where) const
{
    where << "[";
    for (auto i = m_data.begin(); i != m_data.end(); ++i)
    {
        where << std::to_string(*i);
        auto i2 = i;
        i2++;
        if (i2 != m_data.end())
        {
            where << ", ";
        }
    }
    where << "]";
}

class OpenGymTupleContainer : public OpenGymDataContainer
{
  public:
    OpenGymTupleContainer();
    ~OpenGymTupleContainer() override;

    static TypeId GetTypeId();

    ns3_ai_gym::DataContainer GetDataContainerPbMsg() override;

    void Print(std::ostream& where) const override;

    friend std::ostream& operator<<(std::ostream& os, const Ptr<OpenGymTupleContainer> container)
    {
        container->Print(os);
        return os;
    }

    bool Add(Ptr<OpenGymDataContainer> space);
    Ptr<OpenGymDataContainer> Get(uint32_t idx);

  protected:
    // Inherited
    void DoInitialize() override;
    void DoDispose() override;

    std::vector<Ptr<OpenGymDataContainer>> m_tuple;
};

class OpenGymDictContainer : public OpenGymDataContainer
{
  public:
    OpenGymDictContainer();
    ~OpenGymDictContainer() override;

    static TypeId GetTypeId();

    ns3_ai_gym::DataContainer GetDataContainerPbMsg() override;

    void Print(std::ostream& where) const override;

    friend std::ostream& operator<<(std::ostream& os, const Ptr<OpenGymDictContainer> container)
    {
        container->Print(os);
        return os;
    }

    bool Add(std::string key, Ptr<OpenGymDataContainer> value);
    Ptr<OpenGymDataContainer> Get(std::string key);

  protected:
    // Inherited
    void DoInitialize() override;
    void DoDispose() override;

    std::map<std::string, Ptr<OpenGymDataContainer>> m_dict;
};

} // end of namespace ns3

#endif /* OPENGYM_CONTAINER_H */
