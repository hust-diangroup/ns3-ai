
#include "ns3-ai-msg-interface.h"

namespace ns3
{

void
Ns3AiMsgInterface::SetIsMemoryCreator(bool isMemoryCreator)
{
    this->m_isMemoryCreator = isMemoryCreator;
}

void
Ns3AiMsgInterface::SetUseVector(bool useVector)
{
    this->m_useVector = useVector;
}

void
Ns3AiMsgInterface::SetHandleFinish(bool handleFinish)
{
    this->m_handleFinish = handleFinish;
}

void
Ns3AiMsgInterface::SetMemorySize(uint32_t size)
{
    this->m_size = size;
}

void
Ns3AiMsgInterface::SetNames(std::string segmentName,
                            std::string cpp2pyMsgName,
                            std::string py2cppMsgName,
                            std::string lockableName)
{
    this->m_segmentName = segmentName;
    this->m_cpp2pyMsgName = cpp2pyMsgName;
    this->m_py2cppMsgName = py2cppMsgName;
    this->m_lockableName = lockableName;
}

}
