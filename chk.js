// チェック用

function Uploading()
{
	document.getElementById("msg1").innerHTML = "<table style=\"background-color: #ffffff; border-width: 0px 0px 0px 0px;\">"
		+"<tbody style=\"background-color: #ffffff; border-width: 0px 0px 0px 0px;\">"
		+"<tr style=\"background-color: #ffffff; border-width: 0px 0px 0px 0px;\">"
		+"<td style=\"background-color: #ffffff; border-width: 0px 0px 0px 0px;\"><img src=\"./wait.gif\" border=\"0\" alt=\"くるくる\"></td>"
		+"<td style=\"background-color: #ffffff; border-width: 0px 0px 0px 0px;\">アップロード中です。。。終わるまでいらん事しないでお待ちください。</td>"
		+"</tr></tbody></table><br>\n";
		// "<font color=\"#ff0000\">アップロード中です。。。ブラウザを閉じないでください。</font>";
}

function UplCheck()
{
	if(confirm("アップロードしてよろしいですか？")){
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
	if(confirm("チェックが入ったファイルを削除します。\nよろしいですか？")){
		document.upl.up.disabled = true;
		document.del.dlt.disabled = true;
		return true;
	}
	return false;
}
