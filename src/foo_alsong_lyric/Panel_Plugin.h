class Alsong_Panel : public uie::container_ui_extension
{
public:
	virtual const GUID & get_extension_guid() const;
	virtual void get_name(pfc::string_base & out)const;
	virtual void get_category(pfc::string_base & out)const;
	unsigned get_type () const;

private:
	LRESULT on_message(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

	virtual class_data & get_class_data() const; 
	virtual void get_menu_items (uie::menu_hook_t & p_hook) ;

	static const GUID g_extension_guid;
	void OnContextMenu(HWND hParent);
};
