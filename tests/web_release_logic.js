const rules = typeof require === "function" ? require("../src/melodee/web/static/release-logic.js") : module.exports;
function check(value,message) { if(!value)throw new Error(message); }
check(rules.compare('v0.3.0-tapeworkshop-community.100','v0.3.0-tapeworkshop-community.99')===1,'numeric community versions');
check(rules.compare('v0.3.1-tapeworkshop-community.1','v0.3.0-tapeworkshop-community.99')===1,'core version');
check(rules.compare('dev','v0.3.0-tapeworkshop-community.79')===null,'unknown build');
const release={tag_name:'v0.3.0-tapeworkshop-community.80',name:'v80',assets:[{id:123,name:'Tape-Workshop-v80-Signed-Update.zip',size:1000}]};
let found=rules.offer(release,'v0.3.0-tapeworkshop-community.79');
check(found.order===1 && found.api.endsWith('/123') && found.mirror.endsWith('/Tape-Workshop-v80-Signed-Update.zip'),'new signed update');
check(rules.offer({...release,prerelease:true},'v0.3.0-tapeworkshop-community.79')===null,'prerelease skipped');
check(rules.offer({...release,assets:[{id:123,name:'other.zip',size:1000}]},'v0.3.0-tapeworkshop-community.79').api===null,'unsigned name skipped');
check(rules.offer({...release,tag_name:'<script>'},'v0.3.0-tapeworkshop-community.79')===null,'invalid tag');
check(rules.offer({...release,assets:[{id:123,name:'Tape-Workshop-v80-Signed-Update.zip',size:999999999}]},'v0.3.0-tapeworkshop-community.79').api===null,'size cap');
