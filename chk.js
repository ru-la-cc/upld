// �`�F�b�N�p

function Uploading()
{
	document.getElementById("msg1").innerHTML = "<table style=\"background-color: #ffffff; border-width: 0px 0px 0px 0px;\">"
		+"<tbody style=\"background-color: #ffffff; border-width: 0px 0px 0px 0px;\">"
		+"<tr style=\"background-color: #ffffff; border-width: 0px 0px 0px 0px;\">"
		+"<td style=\"background-color: #ffffff; border-width: 0px 0px 0px 0px;\"><img src=\"./wait.gif\" border=\"0\" alt=\"���邭��\"></td>"
		+"<td style=\"background-color: #ffffff; border-width: 0px 0px 0px 0px;\">�A�b�v���[�h���ł��B�B�B�I���܂ł���񎖂��Ȃ��ł��҂����������B</td>"
		+"</tr></tbody></table><br>\n";
		// "<font color=\"#ff0000\">�A�b�v���[�h���ł��B�B�B�u���E�U����Ȃ��ł��������B</font>";
}

function UplCheck()
{
	if(confirm("�A�b�v���[�h���Ă�낵���ł����H")){
		// document.upl.upfile.disabled = true;
		document.upl.up.disabled = true;
		document.del.dlt.disabled = true;
		Uploading();
		return true;
	}
	return false;
}

function DelCheck()
{
	if(confirm("�`�F�b�N���������t�@�C�����폜���܂��B\n��낵���ł����H")){
		document.upl.up.disabled = true;
		document.del.dlt.disabled = true;
		return true;
	}
	return false;
}
