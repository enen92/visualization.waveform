/*
 *      Copyright (C) 2008-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

// Waveform.vis
// A simple visualisation example by MrC

#include <kodi/addon-instance/Visualization.h>
#include <stdio.h>
#ifdef HAS_OPENGL
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#else
#ifdef _WIN32
#include <d3d11_1.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <stdio.h>
#endif
#endif

#ifndef HAS_OPENGL

using namespace DirectX;
using namespace DirectX::PackedVector;

// Include the precompiled shader code.
namespace
{
  #include "DefaultPixelShader.inc"
  #include "DefaultVertexShader.inc"
}

struct cbViewPort
{
  float g_viewPortWidth;
  float g_viewPortHeigh;
  float align1, align2;
};
#endif

#ifdef HAS_OPENGL
typedef struct {
  int TopLeftX;
  int TopLeftY;
  int Width;
  int Height;
  int MinDepth;
  int MaxDepth;
} D3D11_VIEWPORT;
typedef unsigned long D3DCOLOR;
#endif

struct Vertex_t
{
  float x, y, z;
#ifdef HAS_OPENGL
  D3DCOLOR  col;
#else
  XMFLOAT4 col;
#endif
};

class CVisualizationWaveForm
  : public kodi::addon::CAddonBase,
    public kodi::addon::CInstanceVisualization
{
public:
  CVisualizationWaveForm();
  virtual ~CVisualizationWaveForm();

  virtual ADDON_STATUS Create() override;
  virtual void Render() override;
  virtual void AudioData(const float* audioData, int audioDataLength, float *freqData, int freqDataLength) override;

private:
#ifndef HAS_OPENGL
  bool init_renderer_objs();
#endif

#ifndef HAS_OPENGL
  ID3D11Device* m_device;
  ID3D11DeviceContext* m_context;
  ID3D11VertexShader* m_vShader;
  ID3D11PixelShader* m_pShader;
  ID3D11InputLayout* m_inputLayout;
  ID3D11Buffer* m_vBuffer;
  ID3D11Buffer* m_cViewPort;
#else
  void* m_device;
#endif
  float m_fWaveform[2][512];
  D3D11_VIEWPORT m_viewport;
};

CVisualizationWaveForm::CVisualizationWaveForm()
{
  m_device = nullptr;
#ifndef HAS_OPENGL
  m_context = nullptr;
  m_vShader = nullptr;
  m_pShader = nullptr;
  m_inputLayout = nullptr;
  m_vBuffer = nullptr;
  m_cViewPort = nullptr;
#endif
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
CVisualizationWaveForm::~CVisualizationWaveForm()
{
#ifndef HAS_OPENGL
  if (m_cViewPort)
    m_cViewPort->Release();
  if (m_vBuffer)
    m_vBuffer->Release();
  if (m_inputLayout)
    m_inputLayout->Release();
  if (m_vShader)
    m_vShader->Release();
  if (m_pShader)
    m_pShader->Release();
  if (m_device)
    m_device->Release();
#endif
}

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS CVisualizationWaveForm::Create()
{
#ifdef HAS_OPENGL
  m_device = Device();
#endif
  m_viewport.TopLeftX = -1;
  m_viewport.TopLeftY = -1;
  m_viewport.Width = 2;
  m_viewport.Height = 2;
  m_viewport.MinDepth = 0;
  m_viewport.MaxDepth = 1;
#ifndef HAS_OPENGL  
  m_context = (ID3D11DeviceContext*)Device();
  m_context->GetDevice(&m_device);
  if (!init_renderer_objs())
    return ADDON_STATUS_PERMANENT_FAILURE;
#endif

  return ADDON_STATUS_OK;
}

//-- Audiodata ----------------------------------------------------------------
// Called by XBMC to pass new audio data to the vis
//-----------------------------------------------------------------------------
void CVisualizationWaveForm::AudioData(const float* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  int ipos=0;
  while (ipos < 512)
  {
    for (int i=0; i < iAudioDataLength; i+=2)
    {
      m_fWaveform[0][ipos] = pAudioData[i  ]; // left channel
      m_fWaveform[1][ipos] = pAudioData[i+1]; // right channel
      ipos++;
      if (ipos >= 512) break;
    }
  }
}


//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
void CVisualizationWaveForm::Render()
{
  Vertex_t  verts[512];

#ifndef HAS_OPENGL
  unsigned stride = sizeof(Vertex_t), offset = 0;
  m_context->IASetVertexBuffers(0, 1, &m_vBuffer, &stride, &offset);
  m_context->IASetInputLayout(m_inputLayout);
  m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
  m_context->VSSetShader(m_vShader, 0, 0);
  m_context->VSSetConstantBuffers(0, 1, &m_cViewPort);
  m_context->PSSetShader(m_pShader, 0, 0);
  float xcolor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
#endif

  // Left channel
#ifdef HAS_OPENGL
  GLenum errcode;
  glColor3f(1.0, 1.0, 1.0);
  glDisable(GL_BLEND);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  glTranslatef(0,0,-1.0);
  glBegin(GL_LINE_STRIP);
#endif
  for (int i = 0; i < 256; i++)
  {
#ifdef HAS_OPENGL
    verts[i].col = 0xffffffff;
#else
    verts[i].col = XMFLOAT4(xcolor);;
#endif
    verts[i].x = m_viewport.TopLeftX + ((i / 255.0f) * m_viewport.Width);
    verts[i].y = m_viewport.TopLeftY + m_viewport.Height * 0.33f + (m_fWaveform[0][i] * m_viewport.Height * 0.15f);
    verts[i].z = 1.0;
#ifdef HAS_OPENGL
    glVertex2f(verts[i].x, verts[i].y);
#endif
  }

#ifdef HAS_OPENGL
  glEnd();
  if ((errcode=glGetError())!=GL_NO_ERROR) {
    printf("Houston, we have a GL problem: %s\n", gluErrorString(errcode));
  }
#endif

  // Right channel
#ifdef HAS_OPENGL
  glBegin(GL_LINE_STRIP);
  for (int i = 0; i < 256; i++)
#else
  for (int i = 256; i < 512; i++)
#endif
  {
#ifdef HAS_OPENGL
    verts[i].col = 0xffffffff;
    verts[i].x = m_viewport.TopLeftX + ((i / 255.0f) * m_viewport.Width);
#else
    verts[i].col = XMFLOAT4(xcolor);
    verts[i].x = m_viewport.TopLeftX + (((i - 256) / 255.0f) * m_viewport.Width);
#endif
    verts[i].y = m_viewport.TopLeftY + m_viewport.Height * 0.66f + (m_fWaveform[1][i] * m_viewport.Height * 0.15f);
    verts[i].z = 1.0;
#ifdef HAS_OPENGL
    glVertex2f(verts[i].x, verts[i].y);
#endif
  }

#ifdef HAS_OPENGL
  glEnd();
  glEnable(GL_BLEND);


  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  if ((errcode=glGetError())!=GL_NO_ERROR) {
    printf("Houston, we have a GL problem: %s\n", gluErrorString(errcode));
  }
#elif !defined(HAS_OPENGL)
  // a little optimization: generate and send all vertecies for both channels
  D3D11_MAPPED_SUBRESOURCE res;
  if (S_OK == m_context->Map(m_vBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res))
  {
    memcpy(res.pData, verts, sizeof(Vertex_t) * 512);
    m_context->Unmap(m_vBuffer, 0);
  }
  // draw left channel
  m_context->Draw(256, 0);
  // draw right channel
  m_context->Draw(256, 256);
#endif
}

#ifndef HAS_OPENGL
bool CVisualizationWaveForm::init_renderer_objs()
{
  // Create vertex shader
  if (S_OK != m_device->CreateVertexShader(DefaultVertexShaderCode, sizeof(DefaultVertexShaderCode), nullptr, &m_vShader))
    return false;

  // Create input layout
  D3D11_INPUT_ELEMENT_DESC layout[] =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  };
  if (S_OK != m_device->CreateInputLayout(layout, ARRAYSIZE(layout), DefaultVertexShaderCode, sizeof(DefaultVertexShaderCode), &m_inputLayout))
    return false;

  // Create pixel shader
  if (S_OK != m_device->CreatePixelShader(DefaultPixelShaderCode, sizeof(DefaultPixelShaderCode), nullptr, &m_pShader))
    return false;

  // create buffers
  CD3D11_BUFFER_DESC desc(sizeof(Vertex_t) * 512, D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
  if (S_OK != m_device->CreateBuffer(&desc, NULL, &m_vBuffer))
    return false;

  desc.ByteWidth = sizeof(cbViewPort);
  desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.CPUAccessFlags = 0;

  cbViewPort viewPort = { (float)m_viewport.Width, (float)m_viewport.Height, 0.0f, 0.0f };
  D3D11_SUBRESOURCE_DATA initData;
  initData.pSysMem = &viewPort;

  if (S_OK != m_device->CreateBuffer(&desc, &initData, &m_cViewPort))
    return false;

  // we are ready
  return true;
}
#endif // !HAS_OPENGL

ADDONCREATOR(CVisualizationWaveForm)
