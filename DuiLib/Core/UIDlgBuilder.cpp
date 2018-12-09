#include "StdAfx.h"

namespace DuiLib {

	CDialogBuilder::CDialogBuilder () {}

	CControlUI* CDialogBuilder::Create (std::variant<UINT, string_t> xml, string_view_t type, IDialogBuilderCallback* pCallback,
		CPaintManagerUI* pManager, CControlUI* pParent) {
		//��ԴIDΪ0-65535�������ֽڣ��ַ���ָ��Ϊ4���ֽ�
		//�ַ�����<��ͷ��Ϊ��XML�ַ�����������Ϊ��XML�ļ�
		if (xml.index () == 1 && std::get<1> (xml)[0] != _T ('<')) {
			string_view_t xmlpath = CResourceManager::GetInstance ()->GetXmlPath (std::get<1> (xml));
			if (!xmlpath.empty ()) {
				xml = string_t (xmlpath);
			}
		}

		if (xml.index () == 1) {
			if (std::get<1> (xml)[0] == _T ('<')) {
				if (!m_xml.Load (std::get<1> (xml).c_str ())) return nullptr;
			} else {
				if (!m_xml.LoadFromFile (std::get<1> (xml).c_str ())) return nullptr;
			}
		} else {
			HINSTANCE dll_instence = nullptr;
			if (m_instance) {
				dll_instence = m_instance;
			} else {
				dll_instence = CPaintManagerUI::GetResourceDll ();
			}
			HRSRC hResource = ::FindResource (dll_instence, MAKEINTRESOURCE (std::get<0> (xml)), type.data ());
			if (!hResource) return nullptr;
			HGLOBAL hGlobal = ::LoadResource (dll_instence, hResource);
			if (!hGlobal) {
				FreeResource (hResource);
				return nullptr;
			}

			m_pCallback = pCallback;
			if (!m_xml.LoadFromMem ((BYTE*)::LockResource (hGlobal), ::SizeofResource (dll_instence, hResource))) return nullptr;
			::FreeResource (hResource);
			m_pstrtype = type;
		}

		return Create (pCallback, pManager, pParent);
	}

	CControlUI* CDialogBuilder::Create (IDialogBuilderCallback* pCallback, CPaintManagerUI* pManager, CControlUI* pParent) {
		m_pCallback = pCallback;
		CMarkupNode root = m_xml.GetRoot ();
		if (!root.IsValid ()) return nullptr;

		if (pManager) {
			string_t pstrClass = _T ("");
			int nAttributes = 0;
			string_t pstrName = _T ("");
			string_t pstrValue = _T ("");
			for (CMarkupNode node = root.GetChild (); node.IsValid (); node = node.GetSibling ()) {
				pstrClass = node.GetName ();
				if (pstrClass == _T ("Image")) {
					nAttributes = node.GetAttributeCount ();
					string_t pImageName = _T ("");
					string_t pImageResType = _T ("");
					bool shared = false;
					DWORD mask = 0;
					for (int i = 0; i < nAttributes; i++) {
						pstrName = node.GetAttributeName (i);
						pstrValue = node.GetAttributeValue (i);
						if (pstrName == _T ("name")) {
							pImageName = pstrValue;
						} else if (pstrName == _T ("restype")) {
							pImageResType = pstrValue;
						} else if (pstrName == _T ("mask")) {
							mask = (DWORD) FawTools::parse_hex (pstrValue);
						} else if (pstrName == _T ("shared")) {
							shared = FawTools::parse_bool (pstrValue);
						}
					}
					if (!pImageName.empty ()) pManager->AddImage (pImageName, pImageResType, mask, false, shared);
				} else if (pstrClass == _T ("Font")) {
					nAttributes = node.GetAttributeCount ();
					int id = -1;
					string_t pFontName = _T ("");
					int size = 12;
					bool bold = false;
					bool underline = false;
					bool italic = false;
					bool defaultfont = false;
					bool shared = false;
					for (int i = 0; i < nAttributes; i++) {
						pstrName = node.GetAttributeName (i);
						pstrValue = node.GetAttributeValue (i);
						if (pstrName == _T ("id")) {
							id = FawTools::parse_dec (pstrValue);
						} else if (pstrName == _T ("name")) {
							pFontName = pstrValue;
						} else if (pstrName == _T ("size")) {
							size = FawTools::parse_dec (pstrValue);
						} else if (pstrName == _T ("bold")) {
							bold = FawTools::parse_bool (pstrValue);
						} else if (pstrName == _T ("underline")) {
							underline = FawTools::parse_bool (pstrValue);
						} else if (pstrName == _T ("italic")) {
							italic = FawTools::parse_bool (pstrValue);
						} else if (pstrName == _T ("default")) {
							defaultfont = FawTools::parse_bool (pstrValue);
						} else if (pstrName == _T ("shared")) {
							shared = FawTools::parse_bool (pstrValue);
						}
					}
					if (id >= 0) {
						pManager->AddFont (id, pFontName, size, bold, underline, italic, shared);
						if (defaultfont) pManager->SetDefaultFont (pFontName, pManager->GetDPIObj ()->Scale (size), bold, underline, italic, shared);
					}
				} else if (pstrClass == _T ("Default")) {
					nAttributes = node.GetAttributeCount ();
					string_t pControlName = _T ("");
					string_t pControlValue = _T ("");
					bool shared = false;
					for (int i = 0; i < nAttributes; i++) {
						pstrName = node.GetAttributeName (i);
						pstrValue = node.GetAttributeValue (i);
						if (pstrName == _T ("name")) {
							pControlName = pstrValue;
						} else if (pstrName == _T ("value")) {
							pControlValue = pstrValue;
						} else if (pstrName == _T ("shared")) {
							shared = FawTools::parse_bool (pstrValue);
						}
					}
					if (!pControlName.empty ()) {
						pManager->AddDefaultAttributeList (pControlName, pControlValue, shared);
					}
				} else if (pstrClass == _T ("Style")) {
					nAttributes = node.GetAttributeCount ();
					string_t pName = _T ("");
					string_t pStyle = _T ("");
					bool shared = false;
					for (int i = 0; i < nAttributes; i++) {
						pstrName = node.GetAttributeName (i);
						pstrValue = node.GetAttributeValue (i);
						if (pstrName == _T ("name")) {
							pName = pstrValue;
						} else if (pstrName == _T ("value")) {
							pStyle = pstrValue;
						} else if (pstrName == _T ("shared")) {
							shared = FawTools::parse_bool (pstrValue);
						}
					}
					if (!pName.empty ()) {
						pManager->AddStyle (pName, pStyle, shared);
					}
				} else if (pstrClass == _T ("Import")) {
					nAttributes = node.GetAttributeCount ();
					string_t pstrPath = _T ("");
					for (int i = 0; i < nAttributes; i++) {
						pstrName = node.GetAttributeName (i);
						pstrValue = node.GetAttributeValue (i);
						if (pstrName == _T ("fontfile")) {
							pstrPath = pstrValue;
						}
					}
					if (!pstrPath.empty ()) {
						pManager->AddFontArray (pstrPath);
					}
				}
			}

			pstrClass = root.GetName ();
			if (pstrClass == _T ("Window")) {
				if (pManager->GetPaintWindow ()) {
					nAttributes = root.GetAttributeCount ();
					for (int i = 0; i < nAttributes; i++) {
						pstrName = root.GetAttributeName (i);
						pstrValue = root.GetAttributeValue (i);
						if (pstrName == _T ("size")) {
							SIZE sz = FawTools::parse_size (pstrValue);
							pManager->SetInitSize (pManager->GetDPIObj ()->Scale (sz.cx), pManager->GetDPIObj ()->Scale (sz.cy));
						} else if (pstrName == _T ("sizebox")) {
							RECT rcSizeBox = FawTools::parse_rect (pstrValue);
							pManager->SetSizeBox (rcSizeBox);
						} else if (pstrName == _T ("caption")) {
							RECT rcCaption = FawTools::parse_rect (pstrValue);
							pManager->SetCaptionRect (rcCaption);
						} else if (pstrName == _T ("roundcorner")) {
							SIZE sz = FawTools::parse_size (pstrValue);
							pManager->SetRoundCorner (sz.cx, sz.cy);
						} else if (pstrName == _T ("mininfo")) {
							SIZE sz = FawTools::parse_size (pstrValue);
							pManager->SetMinInfo (sz.cx, sz.cy);
						} else if (pstrName == _T ("maxinfo")) {
							SIZE sz = FawTools::parse_size (pstrValue);
							pManager->SetMaxInfo (sz.cx, sz.cy);
						} else if (pstrName == _T ("showdirty")) {
							pManager->SetShowUpdateRect (FawTools::parse_bool (pstrValue));
						} else if (pstrName == _T ("opacity") || pstrName == _T ("alpha")) {
							pManager->SetOpacity ((BYTE) FawTools::parse_dec (pstrValue));
						} else if (pstrName == _T ("layeredopacity")) {
							pManager->SetLayeredOpacity ((BYTE) FawTools::parse_dec (pstrValue));
						} else if (pstrName == _T ("layered") || pstrName == _T ("bktrans")) {
							pManager->SetLayered (FawTools::parse_bool (pstrValue));
						} else if (pstrName == _T ("layeredimage")) {
							pManager->SetLayered (true);
							pManager->SetLayeredImage (pstrValue);
						} else if (pstrName == _T ("noactivate")) {
							pManager->SetNoActivate (FawTools::parse_bool (pstrValue));
						} else if (pstrName == _T ("disabledfontcolor")) {
							pManager->SetDefaultDisabledColor ((DWORD) FawTools::parse_hex (pstrValue));
						} else if (pstrName == _T ("defaultfontcolor")) {
							pManager->SetDefaultFontColor ((DWORD) FawTools::parse_hex (pstrValue));
						} else if (pstrName == _T ("linkfontcolor")) {
							pManager->SetDefaultLinkFontColor ((DWORD) FawTools::parse_hex (pstrValue));
						} else if (pstrName == _T ("linkhoverfontcolor")) {
							pManager->SetDefaultLinkHoverFontColor ((DWORD) FawTools::parse_hex (pstrValue));
						} else if (pstrName == _T ("selectedcolor")) {
							pManager->SetDefaultSelectedBkColor ((DWORD) FawTools::parse_hex (pstrValue));
						} else if (pstrName == _T ("shadowsize")) {
							pManager->GetShadow ()->SetSize (FawTools::parse_dec (pstrValue));
						} else if (pstrName == _T ("shadowsharpness")) {
							pManager->GetShadow ()->SetSharpness (FawTools::parse_dec (pstrValue));
						} else if (pstrName == _T ("shadowdarkness")) {
							pManager->GetShadow ()->SetDarkness (FawTools::parse_dec (pstrValue));
						} else if (pstrName == _T ("shadowposition")) {
							SIZE sz = FawTools::parse_size (pstrValue);
							pManager->GetShadow ()->SetPosition (sz.cx, sz.cy);
						} else if (pstrName == _T ("shadowcolor")) {
							pManager->GetShadow ()->SetColor ((DWORD) FawTools::parse_hex (pstrValue));
						} else if (pstrName == _T ("shadowcorner")) {
							RECT rcCorner = FawTools::parse_rect (pstrValue);
							pManager->GetShadow ()->SetShadowCorner (rcCorner);
						} else if (pstrName == _T ("shadowimage")) {
							pManager->GetShadow ()->SetImage (pstrValue);
						} else if (pstrName == _T ("showshadow")) {
							pManager->GetShadow ()->ShowShadow (FawTools::parse_bool (pstrValue));
						} else if (pstrName == _T ("gdiplustext")) {
							pManager->SetUseGdiplusText (FawTools::parse_bool (pstrValue));
						} else if (pstrName == _T ("textrenderinghint")) {
							pManager->SetGdiplusTextRenderingHint (FawTools::parse_dec (pstrValue));
						} else if (pstrName == _T ("tooltiphovertime")) {
							pManager->SetHoverTime (FawTools::parse_dec (pstrValue));
						}
					}
				}
			}
		}
		return _Parse (&root, pParent, pManager);
	}

	CMarkup* CDialogBuilder::GetMarkup () {
		return &m_xml;
	}

	string_view_t CDialogBuilder::GetLastErrorMessage () const {
		return m_xml.GetLastErrorMessage ();
	}

	string_view_t CDialogBuilder::GetLastErrorLocation () const {
		return m_xml.GetLastErrorLocation ();
	}

	CControlUI* CDialogBuilder::_Parse (CMarkupNode* pRoot, CControlUI* pParent, CPaintManagerUI* pManager) {
		IContainerUI* pContainer = nullptr;
		CControlUI* pReturn = nullptr;
		for (CMarkupNode node = pRoot->GetChild (); node.IsValid (); node = node.GetSibling ()) {
			string_t pstrClass = node.GetName ();
			if (pstrClass == _T ("Image") || pstrClass == _T ("Font") || pstrClass == _T ("Default") || pstrClass == _T ("Style")) continue;

			CControlUI* pControl = nullptr;
			if (pstrClass == _T ("Import")) continue;
			if (pstrClass == _T ("Include")) {
				if (!node.HasAttributes ()) continue;
				int count = 1;
				//string_t szValue (500, _T ('\0'));
				//SIZE_T cchLen = szValue.length () - 1;
				string_t szValue = node.GetAttributeValue (_T ("count"));
				if (!szValue.empty ())
					count = _ttoi (szValue.c_str ());
				szValue = node.GetAttributeValue (_T ("source"));
				if (!szValue.empty ()) continue;
				for (int i = 0; i < count; i++) {
					CDialogBuilder builder;
					if (!m_pstrtype.empty ()) { // ʹ����Դdll������Դ�ж�ȡ
						WORD id = (WORD) FawTools::parse_dec (szValue);
						pControl = builder.Create ((UINT) id, m_pstrtype, m_pCallback, pManager, pParent);
					} else {
						pControl = builder.Create (szValue, (UINT) 0, m_pCallback, pManager, pParent);
					}
				}
				continue;
			} else {
				CDuiString strClass;
				strClass.Format (_T ("C%sUI"), pstrClass.c_str ());
				pControl = dynamic_cast<CControlUI*>(CControlFactory::GetInstance ()->CreateControl (strClass));

				// �����
				if (!pControl) {
					CStdPtrArray* pPlugins = CPaintManagerUI::GetPlugins ();
					LPCREATECONTROL lpCreateControl = nullptr;
					for (int i = 0; i < pPlugins->GetSize (); ++i) {
						lpCreateControl = (LPCREATECONTROL) pPlugins->GetAt (i);
						if (lpCreateControl) {
							pControl = lpCreateControl (pstrClass);
							if (pControl) break;
						}
					}
				}
				// �ص�����
				if (!pControl && m_pCallback) {
					pControl = m_pCallback->CreateControl (pstrClass);
				}
			}

			if (!pControl) {
#ifdef _DEBUG
				DUITRACE (_T ("δ֪�ؼ�:%s"), pstrClass);
#else
				continue;
#endif
			}

			// Add children
			if (node.HasChildren ()) {
				_Parse (&node, pControl, pManager);
			}
			// Attach to parent
			// ��ΪĳЩ���Ժ͸�������أ�����selected��������Add��������
			CTreeViewUI* pTreeView = nullptr;
			if (pParent && pControl) {
				CTreeNodeUI* pParentTreeNode = static_cast<CTreeNodeUI*>(pParent->GetInterface (_T ("TreeNode")));
				CTreeNodeUI* pTreeNode = static_cast<CTreeNodeUI*>(pControl->GetInterface (_T ("TreeNode")));
				pTreeView = static_cast<CTreeViewUI*>(pParent->GetInterface (_T ("TreeView")));
				// TreeNode�ӽڵ�
				if (pTreeNode) {
					if (pParentTreeNode) {
						pTreeView = pParentTreeNode->GetTreeView ();
						if (!pParentTreeNode->Add (pTreeNode)) {
							delete pTreeNode;
							pTreeNode = nullptr;
							continue;
						}
					} else {
						if (pTreeView) {
							if (!pTreeView->Add (pTreeNode)) {
								delete pTreeNode;
								pTreeNode = nullptr;
								continue;
							}
						}
					}
				}
				// TreeNode�ӿؼ�
				else if (pParentTreeNode) {
					pParentTreeNode->GetTreeNodeHoriznotal ()->Add (pControl);
				}
				// ��ͨ�ؼ�
				else {
					if (!pContainer) pContainer = static_cast<IContainerUI*>(pParent->GetInterface (_T ("IContainer")));
					ASSERT (pContainer);
					if (!pContainer) return nullptr;
					if (!pContainer->Add (pControl)) {
						delete pControl;
						continue;
					}
				}
			}
			if (!pControl) continue;

			// Init default attributes
			if (pManager) {
				if (pTreeView) {
					pControl->SetManager (pManager, pTreeView, true);
				} else {
					pControl->SetManager (pManager, nullptr, false);
				}
				string_view_t pDefaultAttributes = pManager->GetDefaultAttributeList (pstrClass);
				if (!pDefaultAttributes.empty ()) {
					pControl->ApplyAttributeList (pDefaultAttributes);
				}
			}
			// Process attributes
			if (node.HasAttributes ()) {
				TCHAR szValue[500] = { 0 };
				SIZE_T cchLen = lengthof (szValue) - 1;
				// Set ordinary attributes
				int nAttributes = node.GetAttributeCount ();
				for (int i = 0; i < nAttributes; i++) {
					pControl->SetAttribute (node.GetAttributeName (i), node.GetAttributeValue (i));
				}
			}
			if (pManager) {
				if (!pTreeView) {
					pControl->SetManager (nullptr, nullptr, false);
				}
			}
			// Return first item
			if (!pReturn) pReturn = pControl;
		}
		return pReturn;
	}

} // namespace DuiLib