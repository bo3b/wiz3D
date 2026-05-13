#include "StdAfx.h"
#include "D3DDeviceWrapper.h"
#include "D3DSwapChain.h"
#include "MonoCommandBuffer.h"
#include "AdapterFunctions.h"  // DDILog

#include "Commands\CommandSet.h"

namespace {

	Commands::TCmdPtr DeviceViewMono( new Commands::ChangeDeviceView(VIEWINDEX_MONO) );
	Commands::TCmdPtr DeviceViewLeft( new Commands::ChangeDeviceView(VIEWINDEX_LEFT) );
	Commands::TCmdPtr DeviceViewRight( new Commands::ChangeDeviceView(VIEWINDEX_RIGHT) );

} // anonymous namespace

MonoCommandBuffer::MonoCommandBuffer()
	:	CommandBuffer			( )
{
}

MonoCommandBuffer::~MonoCommandBuffer()
{
}

void MonoCommandBuffer::Execute()
{
	_ASSERT( m_pWrapper );	

#ifndef FINAL_RELEASE
	m_pWrapper->m_CBCount++;
	m_pWrapper->m_CBSubIndexCount = 0;
	DumpMessage(m_pWrapper, "######################## [%d] MonoCommandBuffer::Execute ########################", m_pWrapper->m_CBCount);
#endif

	for(auto it = m_LeftBuffer.begin(); it != m_LeftBuffer.end(); it++)
	{	
		WriteCommand( *it );
		(*it)->Execute( m_pWrapper );
#ifndef FINAL_RELEASE
		if( (*it)->State_ & COMMAND_CHANGE_DEVICE_STATE )
			(*it)->UpdateDeviceState( m_pWrapper, &m_pWrapper->m_DeviceStateAtExecute );
		else if( GINFO_DUMP_ON && m_pWrapper->m_DumpType >= dtOnlyRT )
		{
			if( (*it)->State_ & COMMAND_DEPENDED_ON_DEVICE_STATE )
			{
				DumpView dview = (Commands::Command::CurrentView_ != VIEWINDEX_RIGHT) ? dvLeft : dvRight;
				m_pWrapper->DumpTextures(false, dview);
				m_pWrapper->DumpRT(false, false, dview);
				++m_pWrapper->m_CBSubIndexCount;
			}
			else if( (*it)->State_ & COMMAND_DEPENDED_ON_COMPUTE_DEVICE_STATE )
			{
				DumpView dview = (Commands::Command::CurrentView_ != VIEWINDEX_RIGHT) ? dvLeft : dvRight;
				m_pWrapper->DumpCSTextures(false, dview);
				m_pWrapper->DumpUAVs(false, false, dview);
				++m_pWrapper->m_CBSubIndexCount;
			}
			else if( (*it)->State_ & COMMAND_MAY_USE_STEREO_RESOURCES )
				DumpDestParam(*it, false, dvLeft);
		}
#endif
	}	

#ifndef FINAL_RELEASE
	m_pWrapper->m_MonoCommandsCount += m_LeftBuffer.size();
	DumpMessage(m_pWrapper, "######################## end ########################");
#endif
}

void MonoCommandBuffer::ExecuteFinalizeCommand()
{
	if ( m_FinalizeCommand )
	{
#ifndef FINAL_RELEASE
		DumpMessage(m_pWrapper, "######################## finalize ########################");
		m_pWrapper->m_MonoCommandsCount++;
#endif

		if (m_FinalizeCommand->State_ & COMMAND_CHANGE_DEVICE_STATE)
		{
			m_FinalizeCommand->UpdateDeviceState( m_pWrapper, &m_pWrapper->m_DeviceState );
#ifndef FINAL_RELEASE
			m_FinalizeCommand->UpdateDeviceState( m_pWrapper, &m_pWrapper->m_DeviceStateAtExecute );
			m_pWrapper->m_SetRenderTargetCount++;
#endif
		}

		WriteCommand( m_FinalizeCommand );
		m_FinalizeCommand->Execute( m_pWrapper );
	}
}

void MonoCommandBuffer::FlushAll()
{
	{
		CriticalSectionLocker locker(m_pWrapper->m_CSUMCall);	
		Execute();
		ExecuteFinalizeCommand();
	}

	Clear();
}

//////////////////////////////////////////////////////////////////////////

void MonoCommandBuffer::AddChangeStateCommand( Commands::Command* pCmd_ )
{
	pCmd_->UpdateDeviceState( m_pWrapper, &m_pWrapper->m_DeviceState );
	AddMonoCommand( pCmd_ );
}

void MonoCommandBuffer::AddCommandThatMayUseStereoResources( Commands::Command* pCmd_, int type )
{
	bool stereoCmd = pCmd_->IsUsedStereoResources(m_pWrapper);
	if ( !m_pWrapper->IsStereoActive() || type == TT_MONO ) 
	{
		// such optimization (update of only one stereo resource in mono) can provide bugs
		stereoCmd = false;
		AddMonoCommand( new Commands::SetDestResourceType(pCmd_, TT_MONO) );
	}
	else if (type != -1) {
		AddMonoCommand( new Commands::SetDestResourceType(pCmd_, TextureType(type)) );
	}
	
	Commands::TCmdPtr ptr(pCmd_);
	if (stereoCmd)
	{
		m_LeftBuffer.push_back(DeviceViewLeft);
		m_LeftBuffer.push_back(ptr);
		m_LeftBuffer.push_back(DeviceViewRight);
		m_LeftBuffer.push_back(ptr);
		m_LeftBuffer.push_back(DeviceViewMono);
	}
	else {
		m_LeftBuffer.push_back(ptr);
	}
}

void MonoCommandBuffer::AddDrawCommand( Commands::DrawBase* pCmd_ )
{
	AddMonoCommand( pCmd_ );

	Commands::TCmdPtr pSetRTs = m_pWrapper->m_DeviceState.m_OMRenderTargets->Clone();
	AddMonoCommand( new Commands::SetDestResourceType(pSetRTs.get(), TT_MONO) );

	m_bHaveDrawCommand = true;

	// Diagnostic: are draws landing in the mono buffer while a stereo RT/DS
	// is bound? If yes, the per-AddCommand routing in D3DDeviceWrapper picks
	// up the wrong buffer because ProcessCB hasn't switched yet — that's a
	// different bug from "no stereo RT exists" or "shader analyzer broken".
	// Rate-limited (first 10 + every 5000th) and only logs when stereo state
	// is set so a quiet log only means draws never collide with stereo RTs.
	{
		static int s_monoDrawCount         = 0;
		static int s_monoDrawWithStereoRT  = 0;
		static int s_loggedMonoWithStereo  = 0;
		bool stereoBound = m_pWrapper->m_DeviceState.m_IsRTStereo
		                || m_pWrapper->m_DeviceState.m_IsDSStereo
		                || m_pWrapper->m_DeviceState.m_IsUAVStereo;
		++s_monoDrawCount;
		if (stereoBound)
		{
			++s_monoDrawWithStereoRT;
			if (s_loggedMonoWithStereo < 10 || (s_loggedMonoWithStereo % 1000) == 0)
			{
				DDILog("MonoCB::AddDraw[%d]: ROUTED TO MONO WHILE STEREO BOUND  IsRTStereo=%d IsDSStereo=%d IsUAVStereo=%d  totalMonoDrawsWithStereoBound=%d\n",
					s_monoDrawCount,
					(int)m_pWrapper->m_DeviceState.m_IsRTStereo,
					(int)m_pWrapper->m_DeviceState.m_IsDSStereo,
					(int)m_pWrapper->m_DeviceState.m_IsUAVStereo,
					s_monoDrawWithStereoRT);
			}
			++s_loggedMonoWithStereo;
		}
		if (s_monoDrawCount < 10 || (s_monoDrawCount % 10000) == 0)
		{
			DDILog("MonoCB::AddDraw[%d]: totalMonoDraws=%d  ofWhichStereoBound=%d  (this draw stereoBound=%d)\n",
				s_monoDrawCount, s_monoDrawCount, s_monoDrawWithStereoRT, (int)stereoBound);
		}
	}

#ifndef FINAL_RELEASE
	++m_pWrapper->m_DrawCount;
	pCmd_->m_bStereoDraw = false;
#endif
}

void MonoCommandBuffer::AddCommand( Commands::xxSetRenderTargets* pCmd_ )
{
	bool bUsedStereoResources = pCmd_->IsUsedStereoResources(m_pWrapper);
	if ( !bUsedStereoResources && gInfo.RenderTargetBuffering ) 
	{
		// next render target is mono, so we can cache it
		pCmd_->UpdateDeviceState(m_pWrapper, &m_pWrapper->m_DeviceState);
		AddMonoCommand(pCmd_);
	}
	else {
		AddFinalizeCommand(pCmd_, bUsedStereoResources);
	}
}

//////////////////////////////////////////////////////////////////////////
#pragma region Resource

void MonoCommandBuffer::AddCommand( Commands::xxMap* pCmd_ )
{
	if ( pCmd_->CommandId == idResourceMap || pCmd_->CommandId == idDynamicResourceMapDiscard ) {
		AddMonoCommand( new Commands::SetDestResourceType(pCmd_, TT_MONO) );
	}
	AddMonoCommand( pCmd_ );
	m_WantProcess = true;
}

void MonoCommandBuffer::AddCommand( Commands::xxUnmap* pCmd_ )
{
#ifdef ENABLE_BUFFERING_MAP
	// find corresponding map
	ResourceWrapper* pWrp;
	InitWrapper(pCmd_->hResource_, pWrp);
	if (pWrp->m_pCurrentMapCmd && pWrp->m_pCurrentMapCmd->UpdateCommand_) 
	{
		// replace map/unmap command with single UpdateSubresourceUP for convenience and work with it
		AddCommand( pWrp->m_pCurrentMapCmd->UpdateCommand_.get() );
	}
	else 
	{
		AddMonoCommand( pCmd_ );
		m_WantProcess = true;
	}
#else
	AddMonoCommand( pCmd_ );
	m_WantProcess = true;
#endif
}

void MonoCommandBuffer::AddDispatchCommand( Commands::DispatchBase11_0* pCmd_ )
{
	AddMonoCommand( pCmd_ );
}
#pragma endregion
