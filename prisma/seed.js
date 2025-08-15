const {PrismaClient} = require('@prisma/client');
const prisma         = new PrismaClient();

const TAGS = [
    {code: 'context/desk', label: '室內桌前', groupCode: 'context'},
    {code: 'context/commute', label: '通勤中', groupCode: 'context'},
    {code: 'context/outdoor', label: '戶外', groupCode: 'context'},
    {code: 'energy/low', label: '低能量', groupCode: 'energy'},
    {code: 'energy/med', label: '中等能量', groupCode: 'energy'},
    {code: 'energy/high', label: '高能量', groupCode: 'energy'},
    {code: 'focus/low', label: '放空', groupCode: 'focus'},
    {code: 'focus/high', label: '專注', groupCode: 'focus'},
];

const TASKS = [
    {
        description: '散步 15 分鐘',
        suggestedTime: 15,
        tags: {'context/outdoor': 0.7, 'energy/med': 0.6}
    },
    {
        description: '閱讀 20 分鐘',
        suggestedTime: 20,
        tags: {'context/desk': 0.7, 'focus/high': 0.7, 'energy/low': 0.5}
    },
    {
        description: '整理房間',
        suggestedTime: 30,
        tags: {'context/desk': 0.6, 'energy/med': 0.7}
    },
    {
        description: '泡杯咖啡',
        suggestedTime: 10,
        tags: {'context/desk': 0.6, 'energy/low': 0.5}
    },
    {
        description: '寫下三件感恩小事',
        suggestedTime: 10,
        tags: {'context/desk': 0.6, 'focus/low': 0.6}
    },
];

async function upsertTags() {
    for (const t of TAGS) {
        await prisma.tagDim.upsert({
            where: {code: t.code},
            update: {label: t.label, groupCode: t.groupCode, isActive: true},
            create: t,
        });
    }
}

async function getTagIdByCode(code) {
    const tag = await prisma.tagDim.findUnique({where: {code}});
    if (!tag)
        throw new Error(`Tag not found: ${code}`);
    return tag.id;
}

async function createTasksWithWeights() {
    for (const t of TASKS) {
        const task = await prisma.task.create({
            data: {description: t.description, suggestedTime: t.suggestedTime},
            select: {id: true},
        });

        for (const [code, baseWeight] of Object.entries(t.tags)) {
            const tagId = await getTagIdByCode(code);
            await               prisma.taskTagWeight.upsert({
                where:
                    {taskId_tagId: {taskId: task.id, tagId}}, // ← 這裡用 camelCase
                update: {baseWeight},
                create: {taskId: task.id, tagId, baseWeight},
            });
        }
    }
}

async function main() {
    await upsertTags();
    await createTasksWithWeights();
}

main().then(() => prisma.$disconnect()).catch(async (e) => {
    console.error(e);
    await prisma.$disconnect();
    process.exit(1);
});
