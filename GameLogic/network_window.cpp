#include "pch.h"
#include "network_window.h"
#include "GameApp.h"
#include "NetworkClient.h"
#include "main.h"
#include "Server.h"
#include "DX9TextRenderer.h"

NetworkWindow::NetworkWindow(const char* name)
  : GuiWindow(name) {}

void NetworkWindow::Render(bool hasFocus)
{
  GuiWindow::Render(hasFocus);

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

  //
  // Render some Networking stats

  g_editorFont.DrawText2D(m_x + 10, m_y + 30, DEF_FONT_SIZE, "SERVER SeqID : %d", Server::m_sequenceId);

  int diff = Server::m_sequenceId - g_lastProcessedSequenceId;
  g_editorFont.DrawText2D(m_x + 10, m_y + 60, DEF_FONT_SIZE, "Diff         : %d", diff);

  g_editorFont.DrawText2D(m_x + 10, m_y + 45, DEF_FONT_SIZE, "CLIENT SeqID : %d", g_lastProcessedSequenceId);

  g_editorFont.DrawText2D(m_x + 10, m_y + 80, DEF_FONT_SIZE, "Inbox: %d", g_app->m_networkClient->m_inbox.Size());

  int nextSeqId = g_app->m_networkClient->GetNextLetterSeqID();
  g_editorFont.DrawText2D(m_x + 10, m_y + 96, DEF_FONT_SIZE, "First Letter SeqID: %d", nextSeqId);
}
